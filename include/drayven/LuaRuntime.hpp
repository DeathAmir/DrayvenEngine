#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
namespace drayven { class LuaRuntime { public: LuaRuntime(); ~LuaRuntime(); LuaRuntime(const LuaRuntime&)=delete; LuaRuntime& operator=(const LuaRuntime&)=delete; bool open(const std::filesystem::path& library={}); void close(); bool available() const; bool runFile(const std::filesystem::path&,std::string* error=nullptr); bool runBuffer(std::string_view,std::string_view chunkName="drayven",std::string* error=nullptr); private: struct Impl; std::unique_ptr<Impl> m_impl; }; }
