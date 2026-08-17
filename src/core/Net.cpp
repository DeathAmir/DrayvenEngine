#include "drayven/Net.hpp"
#if defined(DRAYVEN_HAS_CURL)
#include <curl/curl.h>
#endif
namespace drayven {
#if defined(DRAYVEN_HAS_CURL)
static size_t writeBody(char*p,size_t s,size_t n,void*u){auto*o=static_cast<std::string*>(u);o->append(p,s*n);return s*n;}
#endif
bool HttpClient::available(){
#if defined(DRAYVEN_HAS_CURL)
 return true;
#else
 return false;
#endif
}
HttpResponse HttpClient::get(std::string_view url,const std::vector<std::string>&headers){HttpResponse r;
#if defined(DRAYVEN_HAS_CURL)
 CURL*c=curl_easy_init();if(!c){r.error="curl_easy_init failed";return r;}std::string u(url);curl_easy_setopt(c,CURLOPT_URL,u.c_str());curl_easy_setopt(c,CURLOPT_FOLLOWLOCATION,1L);curl_easy_setopt(c,CURLOPT_CONNECTTIMEOUT_MS,10000L);curl_easy_setopt(c,CURLOPT_TIMEOUT_MS,30000L);curl_easy_setopt(c,CURLOPT_USERAGENT,"DrayvenEngine/0.2");curl_easy_setopt(c,CURLOPT_WRITEFUNCTION,&writeBody);curl_easy_setopt(c,CURLOPT_WRITEDATA,&r.body);curl_slist*l=nullptr;for(auto&h:headers)l=curl_slist_append(l,h.c_str());if(l)curl_easy_setopt(c,CURLOPT_HTTPHEADER,l);auto code=curl_easy_perform(c);if(code!=CURLE_OK)r.error=curl_easy_strerror(code);curl_easy_getinfo(c,CURLINFO_RESPONSE_CODE,&r.status);if(l)curl_slist_free_all(l);curl_easy_cleanup(c);
#else
 (void)url;(void)headers;r.error="Drayven networking is disabled. Reconfigure with -DDRAYVEN_ENABLE_NETWORKING=ON.";
#endif
 return r;}
}
