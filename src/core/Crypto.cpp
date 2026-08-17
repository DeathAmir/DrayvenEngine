#include "drayven/Crypto.hpp"
#include <array>
#include <stdexcept>
#if defined(DRAYVEN_HAS_MBEDTLS)
#include <mbedtls/gcm.h>
#include <mbedtls/md.h>
#include <mbedtls/pkcs5.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#endif
namespace drayven {
bool Crypto::available(){
#if defined(DRAYVEN_HAS_MBEDTLS)
 return true;
#else
 return false;
#endif
}
#if defined(DRAYVEN_HAS_MBEDTLS)
static std::array<unsigned char,32> derive(std::string_view p,const std::vector<std::uint8_t>&s){std::array<unsigned char,32>k{};auto*i=mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);mbedtls_md_context_t m;mbedtls_md_init(&m);if(mbedtls_md_setup(&m,i,1)!=0||mbedtls_pkcs5_pbkdf2_hmac(&m,(const unsigned char*)p.data(),p.size(),s.data(),s.size(),100000,k.size(),k.data())!=0){mbedtls_md_free(&m);throw std::runtime_error("PBKDF2 failed");}mbedtls_md_free(&m);return k;}
static std::vector<std::uint8_t> randomBytes(size_t n){std::vector<std::uint8_t>o(n);mbedtls_entropy_context e;mbedtls_ctr_drbg_context c;mbedtls_entropy_init(&e);mbedtls_ctr_drbg_init(&c);const char*p="drayven-pack";if(mbedtls_ctr_drbg_seed(&c,mbedtls_entropy_func,&e,(const unsigned char*)p,12)!=0||mbedtls_ctr_drbg_random(&c,o.data(),o.size())!=0){mbedtls_ctr_drbg_free(&c);mbedtls_entropy_free(&e);throw std::runtime_error("secure random generation failed");}mbedtls_ctr_drbg_free(&c);mbedtls_entropy_free(&e);return o;}
#endif
CipherBlob Crypto::encryptAes256Gcm(std::span<const std::uint8_t>plain,std::string_view password){
#if defined(DRAYVEN_HAS_MBEDTLS)
 CipherBlob b;b.nonce=randomBytes(28);std::vector<std::uint8_t>s(b.nonce.begin(),b.nonce.begin()+16);auto k=derive(password,s);b.data.resize(plain.size());b.tag.resize(16);mbedtls_gcm_context g;mbedtls_gcm_init(&g);if(mbedtls_gcm_setkey(&g,MBEDTLS_CIPHER_ID_AES,k.data(),256)!=0||mbedtls_gcm_crypt_and_tag(&g,MBEDTLS_GCM_ENCRYPT,plain.size(),b.nonce.data()+16,12,nullptr,0,plain.data(),b.data.data(),b.tag.size(),b.tag.data())!=0){mbedtls_gcm_free(&g);throw std::runtime_error("AES-256-GCM encryption failed");}mbedtls_gcm_free(&g);return b;
#else
 (void)plain;(void)password;throw std::runtime_error("Mbed TLS is disabled.");
#endif
}
std::vector<std::uint8_t> Crypto::decryptAes256Gcm(const CipherBlob&b,std::string_view password){
#if defined(DRAYVEN_HAS_MBEDTLS)
 if(b.nonce.size()!=28||b.tag.size()!=16)throw std::runtime_error("invalid cipher blob");std::vector<std::uint8_t>s(b.nonce.begin(),b.nonce.begin()+16);auto k=derive(password,s);std::vector<std::uint8_t>p(b.data.size());mbedtls_gcm_context g;mbedtls_gcm_init(&g);if(mbedtls_gcm_setkey(&g,MBEDTLS_CIPHER_ID_AES,k.data(),256)!=0||mbedtls_gcm_auth_decrypt(&g,b.data.size(),b.nonce.data()+16,12,nullptr,0,b.tag.data(),b.tag.size(),b.data.data(),p.data())!=0){mbedtls_gcm_free(&g);throw std::runtime_error("AES-256-GCM authentication failed");}mbedtls_gcm_free(&g);return p;
#else
 (void)b;(void)password;throw std::runtime_error("Mbed TLS is disabled.");
#endif
}
}
