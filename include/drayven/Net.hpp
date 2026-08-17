#pragma once
#include <string>
#include <string_view>
#include <vector>
namespace drayven { struct HttpResponse { long status{0}; std::string body,error; bool ok() const { return error.empty()&&status>=200&&status<300; } }; class HttpClient { public: static bool available(); static HttpResponse get(std::string_view,const std::vector<std::string>& headers={}); }; }
