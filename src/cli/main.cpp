#include "drayven/Build.hpp"
#include "drayven/Drys.hpp"
#include "drayven/PackArchive.hpp"
#include "drayven/Project.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
using namespace drayven; namespace fs=std::filesystem;
static void usage(){std::cout<<"Drayven Compiler 0.2\n  drayvenc new <path> <name> [2d|3d]\n  drayvenc transpile <file.drys> [-o output] [--to cpp|lua] [--harden] [--seed N]\n  drayvenc pack <AssetsDir> <output.dpack> <key>\n  drayvenc build <project.drayven> [--target desktop|android] [--script native|luajit] [--engine-root path] [--sdk path] [--ndk path] [--abi abi] [--api level] [--luajit path] [--upx path] [--no-harden]\n";}
static std::uint64_t u64(const std::string&s){return std::stoull(s,nullptr,0);}
int main(int argc,char**argv){try{if(argc<2){usage();return 0;}std::string cmd=argv[1];
 if(cmd=="new"&&argc>=4){auto k=(argc>=5&&std::string(argv[4])=="3d")?ProjectKind::Game3D:ProjectKind::Game2D;auto c=Project::create(argv[2],argv[3],k);std::ofstream s(c.root/"Scripts/Main.drys");s<<"game Main\nvar speed = 5\nfn start()\n    log(\"Hello from DRYS\")\nend\nfn update(dt)\n    if key_down(\"D\")\n        move_x(speed * dt)\n    end\nend\n";fs::create_directories(c.root/"Assets/UI");std::cout<<Project::filePath(c)<<"\n";return 0;}
 if(cmd=="transpile"&&argc>=3){fs::path in=argv[2],out;drys::TranspileOptions o;for(int i=3;i<argc;++i){std::string a=argv[i];if(a=="-o"&&i+1<argc)out=argv[++i];else if(a=="--to"&&i+1<argc)o.language=std::string(argv[++i])=="lua"?drys::OutputLanguage::Lua:drys::OutputLanguage::Cpp;else if(a=="--harden")o.hardenSymbols=true;else if(a=="--seed"&&i+1<argc)o.seed=u64(argv[++i]);}if(out.empty()){out=in;out.replace_extension(o.language==drys::OutputLanguage::Lua?".lua":".cpp");}auto r=drys::Transpiler{}.transpileFile(in,o);if(!r.ok){for(auto&d:r.diagnostics)std::cerr<<d<<"\n";return 2;}std::ofstream(out,std::ios::binary)<<r.output;std::cout<<out<<"\n";return 0;}
 if(cmd=="pack"&&argc>=5){PackArchive::packDirectory(argv[2],argv[3],argv[4]);std::cout<<argv[3]<<"\n";return 0;}
 if(cmd=="build"&&argc>=3){auto p=Project::load(argv[2]);BuildOptions o;for(int i=3;i<argc;++i){std::string a=argv[i];if(a=="--target"&&i+1<argc)o.target=std::string(argv[++i])=="android"?BuildTarget::Android:BuildTarget::Desktop;else if(a=="--script"&&i+1<argc)o.scriptMode=std::string(argv[++i])=="luajit"?ScriptMode::LuaJit:ScriptMode::Native;else if(a=="--engine-root"&&i+1<argc)o.engineRoot=argv[++i];else if(a=="--sdk"&&i+1<argc)o.sdkRoot=argv[++i];else if(a=="--ndk"&&i+1<argc)o.ndkRoot=argv[++i];else if(a=="--abi"&&i+1<argc)o.abi=argv[++i];else if(a=="--api"&&i+1<argc)o.androidApi=std::stoi(argv[++i]);else if(a=="--luajit"&&i+1<argc)o.luaJit=argv[++i];else if(a=="--upx"&&i+1<argc){o.upx=argv[++i];o.useUpx=true;}else if(a=="--no-harden")o.hardenSymbols=false;else if(a=="--seed"&&i+1<argc)o.seed=u64(argv[++i]);}auto r=BuildPipeline::build(p,o);for(auto&l:r.log)std::cout<<l<<"\n";if(r.ok){std::cout<<"OUTPUT="<<r.output<<"\n";return 0;}return 3;}usage();return 1;}catch(const std::exception&e){std::cerr<<"drayvenc: "<<e.what()<<"\n";return 1;}}
