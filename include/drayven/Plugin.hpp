#pragma once
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#if defined(_WIN32)
 #define DRAYVEN_PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
 #define DRAYVEN_PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
#endif
namespace drayven {
inline constexpr std::uint32_t PluginApiVersion=1;
struct PluginHostApi { std::uint32_t apiVersion{PluginApiVersion}; void (*log)(const char* text){}; };
struct PluginDescriptor { std::uint32_t apiVersion{PluginApiVersion}; const char* name{}; const char* version{}; bool (*onLoad)(const PluginHostApi* host){}; void (*onUnload)(){}; };
using PluginEntry=const PluginDescriptor* (*)(const PluginHostApi*);
class PluginModule { public: PluginModule(); ~PluginModule(); PluginModule(const PluginModule&)=delete; PluginModule& operator=(const PluginModule&)=delete; bool load(const std::filesystem::path&,const PluginHostApi&,std::string* error=nullptr); void unload(); bool loaded() const; const PluginDescriptor* descriptor() const; private: struct Impl; std::unique_ptr<Impl> m_impl; };
}
