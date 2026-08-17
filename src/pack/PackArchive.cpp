#include "drayven/PackArchive.hpp"
#include "drayven/Crypto.hpp"
#include <fstream>
#include <stdexcept>

namespace drayven {
static std::uint64_t fnv64(std::string_view s){std::uint64_t h=1469598103934665603ull;for(unsigned char c:s){h^=c;h*=1099511628211ull;}return h;}
static std::uint64_t splitmix64(std::uint64_t& x){std::uint64_t z=(x+=0x9e3779b97f4a7c15ull);z=(z^(z>>30))*0xbf58476d1ce4e5b9ull;z=(z^(z>>27))*0x94d049bb133111ebull;return z^(z>>31);}
void PackArchive::legacyCrypt(std::vector<std::uint8_t>& data,std::string_view key,std::uint64_t nonce){
    std::uint64_t state=fnv64(key)^nonce,word=0;int left=0;
    for(auto& b:data){if(left==0){word=splitmix64(state);left=8;}b^=static_cast<std::uint8_t>(word&0xff);word>>=8;--left;}
}
static void writeU32(std::ofstream& o,std::uint32_t v){o.write(reinterpret_cast<const char*>(&v),4);}
static void writeU64(std::ofstream& o,std::uint64_t v){o.write(reinterpret_cast<const char*>(&v),8);}
static std::uint32_t readU32(std::ifstream& i){std::uint32_t v{};i.read(reinterpret_cast<char*>(&v),4);return v;}
static std::uint64_t readU64(std::ifstream& i){std::uint64_t v{};i.read(reinterpret_cast<char*>(&v),8);return v;}

void PackArchive::packDirectory(const std::filesystem::path& input,const std::filesystem::path& output,std::string_view key){
    if(!std::filesystem::exists(input)) throw std::runtime_error("pack input does not exist");
    std::filesystem::create_directories(output.parent_path().empty()?std::filesystem::path("."):output.parent_path());

    if(Crypto::available()){
        struct Item{std::string path;CipherBlob blob;};
        std::vector<Item> items;
        for(auto& e:std::filesystem::recursive_directory_iterator(input)){
            if(!e.is_regular_file()) continue;
            std::ifstream in(e.path(),std::ios::binary);
            std::vector<std::uint8_t> b((std::istreambuf_iterator<char>(in)),{});
            auto p=std::filesystem::relative(e.path(),input).generic_string();
            std::string scopedKey(key); scopedKey += "|"; scopedKey += p;
            items.push_back({p,Crypto::encryptAes256Gcm(b,scopedKey)});
        }
        std::ofstream out(output,std::ios::binary);
        out.write("DRPK",4);writeU32(out,2);writeU32(out,static_cast<std::uint32_t>(items.size()));
        for(auto& it:items){
            writeU32(out,static_cast<std::uint32_t>(it.path.size()));
            writeU32(out,static_cast<std::uint32_t>(it.blob.nonce.size()));
            writeU32(out,static_cast<std::uint32_t>(it.blob.tag.size()));
            writeU64(out,it.blob.data.size());
            out.write(it.path.data(),it.path.size());
            out.write(reinterpret_cast<const char*>(it.blob.nonce.data()),it.blob.nonce.size());
            out.write(reinterpret_cast<const char*>(it.blob.tag.data()),it.blob.tag.size());
            out.write(reinterpret_cast<const char*>(it.blob.data.data()),it.blob.data.size());
        }
        return;
    }

    struct Legacy{std::string path;std::vector<std::uint8_t> bytes;std::uint64_t nonce;};
    std::vector<Legacy> items;std::uint64_t n=1;
    for(auto& e:std::filesystem::recursive_directory_iterator(input)){
        if(!e.is_regular_file()) continue;
        std::ifstream in(e.path(),std::ios::binary);
        std::vector<std::uint8_t>b((std::istreambuf_iterator<char>(in)),{});
        auto p=std::filesystem::relative(e.path(),input).generic_string();
        auto nonce=fnv64(p)^(n++*0xD2B74407B1CE6E93ull);
        legacyCrypt(b,key,nonce);
        items.push_back({p,std::move(b),nonce});
    }
    std::ofstream out(output,std::ios::binary);
    out.write("DRPK",4);writeU32(out,1);writeU32(out,static_cast<std::uint32_t>(items.size()));
    for(auto& it:items){writeU32(out,static_cast<std::uint32_t>(it.path.size()));writeU64(out,it.nonce);writeU64(out,it.bytes.size());out.write(it.path.data(),it.path.size());out.write(reinterpret_cast<const char*>(it.bytes.data()),it.bytes.size());}
}

std::vector<PackedFile> PackArchive::read(const std::filesystem::path& archive,std::string_view key){
    std::ifstream in(archive,std::ios::binary);
    char magic[4]{};in.read(magic,4);
    if(std::string(magic,4)!="DRPK") throw std::runtime_error("invalid dpack");
    auto ver=readU32(in);
    auto count=readU32(in);
    std::vector<PackedFile> out;out.reserve(count);
    if(ver==1){
        for(std::uint32_t x=0;x<count;x++){
            auto plen=readU32(in);auto nonce=readU64(in);auto sz=readU64(in);
            std::string p(plen,'\0');in.read(p.data(),plen);
            std::vector<std::uint8_t>b(sz);in.read(reinterpret_cast<char*>(b.data()),sz);
            legacyCrypt(b,key,nonce);out.push_back({std::move(p),std::move(b)});
        }
        return out;
    }
    if(ver!=2) throw std::runtime_error("unsupported dpack version");
    if(!Crypto::available()) throw std::runtime_error("secure dpack requires Drayven built with Mbed TLS");
    for(std::uint32_t x=0;x<count;x++){
        auto plen=readU32(in);auto nlen=readU32(in);auto tlen=readU32(in);auto sz=readU64(in);
        std::string p(plen,'\0');in.read(p.data(),plen);
        CipherBlob blob;blob.nonce.resize(nlen);blob.tag.resize(tlen);blob.data.resize(sz);
        in.read(reinterpret_cast<char*>(blob.nonce.data()),blob.nonce.size());
        in.read(reinterpret_cast<char*>(blob.tag.data()),blob.tag.size());
        in.read(reinterpret_cast<char*>(blob.data.data()),blob.data.size());
        std::string scopedKey(key);scopedKey+="|";scopedKey+=p;
        out.push_back({std::move(p),Crypto::decryptAes256Gcm(blob,scopedKey)});
    }
    return out;
}
}
