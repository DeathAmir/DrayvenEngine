#include "drayven/Project.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace drayven {
static std::string esc(std::string s) {
    std::string out; out.reserve(s.size());
    for(char c: s) { if(c=='\\' || c=='\"') out.push_back('\\'); out.push_back(c); }
    return out;
}
static std::string valueOf(const std::string& text, const std::string& key) {
    auto k = text.find("\""+key+"\""); if(k==std::string::npos) return {};
    auto c = text.find(':', k); if(c==std::string::npos) return {};
    auto p = text.find_first_not_of(" \t\r\n", c+1); if(p==std::string::npos) return {};
    if(text[p]=='"') { auto q2=text.find('"', p+1); if(q2!=std::string::npos) return text.substr(p+1,q2-p-1); return {}; }
    auto e = text.find_first_of(",}\n", p); auto v=text.substr(p, e==std::string::npos?std::string::npos:e-p);
    auto a=v.find_first_not_of(" \t\r\n"); auto b=v.find_last_not_of(" \t\r\n"); return a==std::string::npos?std::string{}:v.substr(a,b-a+1);
}
ProjectConfig Project::create(const std::filesystem::path& root, std::string name, ProjectKind kind) {
    ProjectConfig c; c.root=std::filesystem::absolute(root); c.name=std::move(name); c.kind=kind;
    std::filesystem::create_directories(c.root / "Assets/Textures");
    std::filesystem::create_directories(c.root / "Assets/Audio");
    std::filesystem::create_directories(c.root / "Assets/Fonts");
    std::filesystem::create_directories(c.root / "Scripts");
    std::filesystem::create_directories(c.root / "Scenes");
    std::filesystem::create_directories(c.root / "Build");
    save(c); return c;
}
ProjectConfig Project::load(const std::filesystem::path& file) {
    std::ifstream in(file); if(!in) throw std::runtime_error("cannot open project: "+file.string());
    std::stringstream ss; ss<<in.rdbuf(); auto t=ss.str(); ProjectConfig c; c.root=std::filesystem::absolute(file.parent_path());
    c.name=valueOf(t,"name"); auto kind=valueOf(t,"kind"); c.kind=kind=="3d"?ProjectKind::Game3D:ProjectKind::Game2D;
    auto startup=valueOf(t,"startupScene"); if(!startup.empty()) c.startupScene=startup;
    auto key=valueOf(t,"assetKey"); if(!key.empty()) c.assetKey=key;
    auto w=valueOf(t,"width"); auto h=valueOf(t,"height"); if(!w.empty()) c.width=std::stoi(w); if(!h.empty()) c.height=std::stoi(h);
    return c;
}
void Project::save(const ProjectConfig& c) {
    std::filesystem::create_directories(c.root); std::ofstream out(filePath(c));
    out << "{\n  \"format\": 1,\n  \"name\": \""<<esc(c.name)<<"\",\n  \"kind\": \""<<(c.kind==ProjectKind::Game3D?"3d":"2d")<<"\",\n"
        << "  \"startupScene\": \""<<esc(c.startupScene.generic_string())<<"\",\n  \"assetKey\": \""<<esc(c.assetKey)<<"\",\n"
        << "  \"width\": "<<c.width<<",\n  \"height\": "<<c.height<<"\n}\n";
}
std::filesystem::path Project::filePath(const ProjectConfig& c){ return c.root/(c.name+".drayven"); }
}
