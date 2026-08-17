#include "drayven/LuaRuntime.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

struct lua_State;

namespace drayven {
namespace {
#ifdef _WIN32
using LibHandle = HMODULE;
static LibHandle openLib(const std::filesystem::path& p) { return LoadLibraryW(p.wstring().c_str()); }
static void closeLib(LibHandle h) { if (h) FreeLibrary(h); }
static void* sym(LibHandle h, const char* n) { return reinterpret_cast<void*>(GetProcAddress(h, n)); }
#else
using LibHandle = void*;
static LibHandle openLib(const std::filesystem::path& p) { return dlopen(p.c_str(), RTLD_NOW | RTLD_LOCAL); }
static void closeLib(LibHandle h) { if (h) dlclose(h); }
static void* sym(LibHandle h, const char* n) { return dlsym(h, n); }
#endif
using LuaPCall = int (*)(lua_State*, int, int, int);
using LuaToLString = const char* (*)(lua_State*, int, std::size_t*);
static bool executeLua(lua_State* state, int loadResult, LuaPCall pcall, LuaToLString toString, std::string* error) {
    if (loadResult != 0) {
        if (error) {
            std::size_t len = 0;
            auto* msg = toString(state, -1, &len);
            *error = msg ? std::string(msg, len) : "LuaJIT load error";
        }
        return false;
    }
    if (pcall(state, 0, -1, 0) != 0) {
        if (error) {
            std::size_t len = 0;
            auto* msg = toString(state, -1, &len);
            *error = msg ? std::string(msg, len) : "LuaJIT runtime error";
        }
        return false;
    }
    return true;
}
}

struct LuaRuntime::Impl {
    using NewState = lua_State* (*)();
    using OpenLibs = void (*)(lua_State*);
    using LoadFile = int (*)(lua_State*, const char*);
    using LoadBuffer = int (*)(lua_State*, const char*, std::size_t, const char*);
    using Close = void (*)(lua_State*);

    LibHandle library{};
    lua_State* state{};
    NewState newState{};
    OpenLibs openLibs{};
    LoadFile loadFile{};
    LoadBuffer loadBuffer{};
    LuaPCall pcall{};
    LuaToLString toLString{};
    Close closeState{};

    template<class T> bool bind(T& out, const char* name) {
        out = reinterpret_cast<T>(sym(library, name));
        return out != nullptr;
    }
};

LuaRuntime::LuaRuntime():m_impl(std::make_unique<Impl>()){}
LuaRuntime::~LuaRuntime(){ close(); }

bool LuaRuntime::open(const std::filesystem::path& preferred) {
    close();
    std::vector<std::filesystem::path> candidates;
    if (!preferred.empty()) candidates.push_back(preferred);
#ifdef _WIN32
    candidates.emplace_back("lua51.dll");
    candidates.emplace_back("bin/lua51.dll");
#else
#if defined(__ANDROID__)
    candidates.emplace_back("libluajit.so");
#else
    candidates.emplace_back("libluajit-5.1.so.2");
    candidates.emplace_back("libluajit-5.1.so");
    candidates.emplace_back("libluajit.so");
    candidates.emplace_back("./bin/libluajit-5.1.so.2");
#endif
#endif
    for (const auto& p : candidates) {
        m_impl->library = openLib(p);
        if (m_impl->library) break;
    }
    if (!m_impl->library) return false;
    bool ok =
        m_impl->bind(m_impl->newState, "luaL_newstate") &&
        m_impl->bind(m_impl->openLibs, "luaL_openlibs") &&
        m_impl->bind(m_impl->loadFile, "luaL_loadfile") &&
        m_impl->bind(m_impl->loadBuffer, "luaL_loadbuffer") &&
        m_impl->bind(m_impl->pcall, "lua_pcall") &&
        m_impl->bind(m_impl->toLString, "lua_tolstring") &&
        m_impl->bind(m_impl->closeState, "lua_close");
    if (!ok) { close(); return false; }
    m_impl->state = m_impl->newState();
    if (!m_impl->state) { close(); return false; }
    m_impl->openLibs(m_impl->state);
    return true;
}

void LuaRuntime::close() {
    if (!m_impl) return;
    if (m_impl->state && m_impl->closeState) m_impl->closeState(m_impl->state);
    m_impl->state = nullptr;
    closeLib(m_impl->library);
    m_impl->library = {};
}

bool LuaRuntime::available() const { return m_impl && m_impl->state; }

bool LuaRuntime::runFile(const std::filesystem::path& file, std::string* error) {
    if (!available()) {
        if (error) *error = "LuaJIT runtime is not loaded";
        return false;
    }
    return executeLua(m_impl->state, m_impl->loadFile(m_impl->state, file.string().c_str()),
                      m_impl->pcall, m_impl->toLString, error);
}

bool LuaRuntime::runBuffer(std::string_view bytes, std::string_view chunkName, std::string* error) {
    if (!available()) {
        if (error) *error = "LuaJIT runtime is not loaded";
        return false;
    }
    std::string name(chunkName);
    return executeLua(m_impl->state,
                      m_impl->loadBuffer(m_impl->state, bytes.data(), bytes.size(), name.c_str()),
                      m_impl->pcall, m_impl->toLString, error);
}
}
