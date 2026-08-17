#include "drayven/Build.hpp"
#include "drayven/PackArchive.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <sstream>
#include <system_error>
namespace drayven { namespace fs=std::filesystem; namespace {
std::string quote(const fs::path& p){
#ifdef _WIN32
 return "\""+p.string()+"\"";
#else
 std::string out="'"; for(char c: p.string()) out += c=='\'' ? "'\\''" : std::string(1,c); return out+"'";
#endif
}
bool run(const std::string& c,std::vector<std::string>& log){log.push_back("$ "+c);int code=std::system(c.c_str());if(code)log.push_back("command failed with code "+std::to_string(code));return code==0;}
void copyTree(const fs::path& a,const fs::path& b){if(!fs::exists(a))return;fs::create_directories(b);for(auto&e:fs::recursive_directory_iterator(a)){auto d=b/fs::relative(e.path(),a);if(e.is_directory())fs::create_directories(d);else if(e.is_regular_file()){fs::create_directories(d.parent_path());fs::copy_file(e.path(),d,fs::copy_options::overwrite_existing);}}}
fs::path envPath(const char*n){if(auto*p=std::getenv(n);p&&*p)return p;return{};}
fs::path resolveEngineRoot(const BuildOptions&o){if(!o.engineRoot.empty())return fs::absolute(o.engineRoot);auto e=envPath("DRAYVEN_ENGINE_ROOT");if(!e.empty())return fs::absolute(e);auto c=fs::current_path();if(fs::exists(c/"CMakeLists.txt")&&fs::exists(c/"include/drayven"))return c;return{};}
bool compileLuaJit(const fs::path&j,const fs::path&i,const fs::path&o,std::vector<std::string>&l){fs::create_directories(o.parent_path());return run(quote(j)+" -b "+quote(i)+" "+quote(o),l);}
}
fs::path BuildPipeline::discoverNdk(const fs::path& sdk){if(sdk.empty()||!fs::exists(sdk/"ndk"))return{};std::vector<fs::path>v;for(auto&e:fs::directory_iterator(sdk/"ndk"))if(e.is_directory())v.push_back(e.path());if(v.empty())return{};std::sort(v.begin(),v.end());return v.back();}
fs::path BuildPipeline::discoverLuaJit(const fs::path&p){if(!p.empty()&&fs::exists(p))return p;auto e=envPath("DRAYVEN_LUAJIT");if(!e.empty()&&fs::exists(e))return e;
#ifdef _WIN32
 for(auto x:{fs::path("luajit.exe"),fs::path("bin/luajit.exe")})
#else
 for(auto x:{fs::path("luajit"),fs::path("bin/luajit"),fs::path("/usr/bin/luajit")})
#endif
 if(fs::exists(x))return x;return{};}
BuildReport BuildPipeline::build(const ProjectConfig&p,const BuildOptions&o){BuildReport r;auto root=p.root,build=root/"Build",native=build/"Generated/Native",lua=build/"Generated/Lua",pack=build/"PackRoot";std::error_code ec;fs::remove_all(pack,ec);fs::create_directories(native);fs::create_directories(lua);fs::create_directories(pack/"Assets");fs::create_directories(pack/"Scripts");copyTree(root/"Assets",pack/"Assets");std::vector<fs::path>sources;auto jit=discoverLuaJit(o.luaJit);auto scripts=root/"Scripts";
 if(fs::exists(scripts))for(auto&e:fs::recursive_directory_iterator(scripts)){if(!e.is_regular_file())continue;auto ext=e.path().extension().string();if(ext==".drys"){drys::TranspileOptions t;t.language=o.scriptMode==ScriptMode::Native?drys::OutputLanguage::Cpp:drys::OutputLanguage::Lua;t.hardenSymbols=o.hardenSymbols;t.seed=o.seed^std::hash<std::string>{}(e.path().generic_string());auto z=drys::Transpiler{}.transpileFile(e.path(),t);if(!z.ok){r.log.insert(r.log.end(),z.diagnostics.begin(),z.diagnostics.end());return r;}if(t.language==drys::OutputLanguage::Cpp){auto f=native/e.path().filename();f.replace_extension(".cpp");std::ofstream(f)<<z.output;sources.push_back(fs::absolute(f));}else{auto f=lua/e.path().filename();f.replace_extension(".lua");std::ofstream(f)<<z.output;if(jit.empty()){r.log.push_back("LuaJIT compiler not found. Set --luajit or DRAYVEN_LUAJIT.");return r;}auto b=pack/"Scripts"/e.path().filename();b.replace_extension(".dluac");if(!compileLuaJit(jit,f,b,r.log))return r;}}else if(ext==".lua"){if(jit.empty()){r.log.push_back("Project contains Lua scripts but LuaJIT compiler was not found.");return r;}auto b=pack/"Scripts"/e.path().filename();b.replace_extension(".dluac");if(!compileLuaJit(jit,e.path(),b,r.log))return r;}}
 fs::create_directories(build);auto dpack=build/"GameAssets.dpack";try{PackArchive::packDirectory(pack,dpack,p.assetKey);}catch(const std::exception&e){r.log.push_back(e.what());return r;}auto engine=resolveEngineRoot(o);if(engine.empty()||!fs::exists(engine/"CMakeLists.txt")){r.log.push_back("Drayven engine root not found. Pass --engine-root or set DRAYVEN_ENGINE_ROOT.");return r;}std::ostringstream ss;for(size_t i=0;i<sources.size();++i){if(i)ss<<';';ss<<sources[i].generic_string();}
 if(o.target==BuildTarget::Android){auto sdk=o.sdkRoot;if(sdk.empty())sdk=envPath("ANDROID_SDK_ROOT");if(sdk.empty())sdk=envPath("ANDROID_HOME");auto ndk=o.ndkRoot;if(ndk.empty())ndk=envPath("ANDROID_NDK_HOME");if(ndk.empty())ndk=discoverNdk(sdk);if(sdk.empty()||!fs::exists(sdk)){r.log.push_back("Android SDK not found. Use --sdk <path> or ANDROID_SDK_ROOT.");return r;}auto tc=ndk/"build/cmake/android.toolchain.cmake";if(ndk.empty()||!fs::exists(tc)){r.log.push_back("Android NDK/toolchain not found. Use --ndk <path>.");return r;}auto out=build/"Android"/o.abi;std::ostringstream c;c<<"cmake -S "<<quote(engine)<<" -B "<<quote(out)<<" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="<<quote(tc)<<" -DANDROID_ABI="<<o.abi<<" -DANDROID_PLATFORM=android-"<<o.androidApi<<" -DDRAYVEN_BUILD_EDITOR=OFF -DDRAYVEN_BUILD_TESTS=OFF -DDRAYVEN_ENABLE_EIGEN=OFF -DDRAYVEN_ENABLE_NETWORKING=OFF -DDRAYVEN_ENABLE_MBEDTLS=ON -DDRAYVEN_ENABLE_VULKAN=OFF -DDRAYVEN_BUILD_SHARED_ENGINE=ON";if(!sources.empty())c<<" -DDRAYVEN_GAME_SOURCES=\""<<ss.str()<<"\"";if(!run(c.str(),r.log)||!run("cmake --build "+quote(out)+" --target DrayvenEngine --config Release --parallel 2",r.log))return r;for(auto&e:fs::recursive_directory_iterator(out))if(e.is_regular_file()&&e.path().filename()=="libDrayvenEngine.so"){r.output=e.path();break;}if(r.output.empty()){r.log.push_back("Android build completed but libDrayvenEngine.so was not located.");return r;}fs::copy_file(dpack,r.output.parent_path()/dpack.filename(),fs::copy_options::overwrite_existing);r.ok=true;r.log.push_back("Android NDK build ready: "+r.output.string());return r;}
 auto out=build/"Desktop";std::ostringstream c;c<<"cmake -S "<<quote(engine)<<" -B "<<quote(out)<<" -DCMAKE_BUILD_TYPE=Release -DDRAYVEN_BUILD_EDITOR=OFF -DDRAYVEN_BUILD_TESTS=OFF -DDRAYVEN_BUILD_SHARED_ENGINE=ON";if(!sources.empty())c<<" -DDRAYVEN_GAME_SOURCES=\""<<ss.str()<<"\"";if(!run(c.str(),r.log)||!run("cmake --build "+quote(out)+" --target DrayvenEngine --config Release --parallel 2",r.log))return r;
#ifdef _WIN32
 const char*name="DrayvenEngine.dll";
#else
 const char*name="libDrayvenEngine.so";
#endif
 for(auto&e:fs::recursive_directory_iterator(out))if(e.is_regular_file()&&e.path().filename()==name){r.output=e.path();break;}if(r.output.empty()){r.log.push_back(std::string("Desktop build completed but ")+name+" was not located.");return r;}fs::copy_file(dpack,r.output.parent_path()/dpack.filename(),fs::copy_options::overwrite_existing);if(o.useUpx&&!o.upx.empty()&&fs::exists(o.upx))if(!run(quote(o.upx)+" --best "+quote(r.output),r.log))r.log.push_back("UPX failed; uncompressed binary remains usable.");r.ok=true;r.log.push_back("Desktop native library ready: "+r.output.string());return r;}
}
