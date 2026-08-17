#include "drayven/Build.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
namespace fs=std::filesystem;
static std::string q(const fs::path&p){
#ifdef _WIN32
 return "\""+p.string()+"\"";
#else
 std::string o="'";for(char c:p.string())o+=c=='\''?"'\\''":std::string(1,c);return o+"'";
#endif
}
int main(int argc,char**argv){if(argc<3){std::cout<<"drayvenluac <input.lua> <output.dluac> [--luajit <path>]\n";return 0;}fs::path in=argv[1],out=argv[2],preferred;for(int i=3;i<argc;++i)if(std::string(argv[i])=="--luajit"&&i+1<argc)preferred=argv[++i];auto jit=drayven::BuildPipeline::discoverLuaJit(preferred);if(jit.empty()){std::cerr<<"LuaJIT executable not found. Use --luajit or DRAYVEN_LUAJIT.\n";return 2;}fs::create_directories(out.parent_path().empty()?fs::path("."):out.parent_path());auto cmd=q(jit)+" -b "+q(in)+" "+q(out);std::cout<<"$ "<<cmd<<"\n";int code=std::system(cmd.c_str());if(code)return code;std::cout<<out<<"\n";return 0;}
