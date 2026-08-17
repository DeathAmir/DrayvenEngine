#include "drayven/Plugin.hpp"
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
namespace drayven { struct PluginModule::Impl {
#ifdef _WIN32
 HMODULE handle{};
#else
 void*handle{};
#endif
 const PluginDescriptor*descriptor{};};
PluginModule::PluginModule():m_impl(std::make_unique<Impl>()){} PluginModule::~PluginModule(){unload();}
bool PluginModule::load(const std::filesystem::path&lib,const PluginHostApi&host,std::string*error){unload();
#ifdef _WIN32
 m_impl->handle=LoadLibraryW(lib.wstring().c_str());if(!m_impl->handle){if(error)*error="LoadLibrary failed";return false;}auto entry=reinterpret_cast<PluginEntry>(GetProcAddress(m_impl->handle,"DrayvenPluginEntry"));
#else
 m_impl->handle=dlopen(lib.c_str(),RTLD_NOW|RTLD_LOCAL);if(!m_impl->handle){if(error){const char*r=dlerror();*error=r?r:"dlopen failed";}return false;}auto entry=reinterpret_cast<PluginEntry>(dlsym(m_impl->handle,"DrayvenPluginEntry"));
#endif
 if(!entry){if(error)*error="missing DrayvenPluginEntry export";unload();return false;}m_impl->descriptor=entry(&host);if(!m_impl->descriptor||m_impl->descriptor->apiVersion!=PluginApiVersion){if(error)*error="plugin ABI mismatch";unload();return false;}if(m_impl->descriptor->onLoad&&!m_impl->descriptor->onLoad(&host)){if(error)*error="plugin onLoad rejected";unload();return false;}return true;}
void PluginModule::unload(){if(!m_impl||!m_impl->handle)return;if(m_impl->descriptor&&m_impl->descriptor->onUnload)m_impl->descriptor->onUnload();
#ifdef _WIN32
 FreeLibrary(m_impl->handle);
#else
 dlclose(m_impl->handle);
#endif
 m_impl->handle={};m_impl->descriptor=nullptr;}
bool PluginModule::loaded()const{return m_impl&&m_impl->handle!=nullptr;}const PluginDescriptor*PluginModule::descriptor()const{return m_impl?m_impl->descriptor:nullptr;}
}
