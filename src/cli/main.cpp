#include "drayven/Drys.hpp"
#include "drayven/PackArchive.hpp"
#include "drayven/Project.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

using namespace drayven;
static void usage(){std::cout<<"Drayven Compiler (drayvenc)\n"
<<"  drayvenc new <path> <name> [2d|3d]\n"
<<"  drayvenc transpile <file.drys> [-o output.cpp]\n"
<<"  drayvenc pack <AssetsDir> <output.dpack> <key>\n"
<<"  drayvenc build <project.drayven> [--target desktop|android]\n";}
static std::string q(const std::filesystem::path&p){return "\""+p.string()+"\"";}
static bool env(const char*n){auto v=std::getenv(n);return v&&*v;}
static int buildProject(const std::filesystem::path& pf,const std::string& target){
    auto cfg=Project::load(pf); auto build=cfg.root/"Build";auto generated=build/"Generated";std::filesystem::create_directories(generated);
    drys::Transpiler tr; std::vector<std::filesystem::path> cpp;
    auto scripts=cfg.root/"Scripts";if(std::filesystem::exists(scripts))for(auto&e:std::filesystem::recursive_directory_iterator(scripts))if(e.is_regular_file()&&e.path().extension()==".drys"){
        auto r=tr.transpileFile(e.path());if(!r.ok){for(auto&d:r.diagnostics)std::cerr<<d<<"\n";return 2;}auto out=generated/(e.path().stem().string()+".cpp");std::ofstream f(out);f<<r.cpp;cpp.push_back(out);
    }
    PackArchive::packDirectory(cfg.root/"Assets",build/"GameAssets.dpack",cfg.assetKey);
    if(target=="android"){
        if(!env("ANDROID_SDK_ROOT")&&!env("ANDROID_HOME")){std::cerr<<"Android SDK missing. Set ANDROID_SDK_ROOT or ANDROID_HOME. Install SDK, NDK and CMake.\n";return 3;}
        auto sdk=std::filesystem::path(std::getenv(env("ANDROID_SDK_ROOT")?"ANDROID_SDK_ROOT":"ANDROID_HOME"));
        auto ndkRoot=sdk/"ndk"; if(!std::filesystem::exists(ndkRoot)){std::cerr<<"Android NDK missing under "<<ndkRoot<<"\n";return 3;}
        std::filesystem::create_directories(build/"Android/app/src/main");
        std::ofstream manifest(build/"Android/app/src/main/AndroidManifest.xml");manifest<<"<manifest xmlns:android=\"http://schemas.android.com/apk/res/android\"><application android:theme=\"@android:style/Theme.Black.NoTitleBar.Fullscreen\" android:label=\""<<cfg.name<<"\"><activity android:name=\"org.libsdl.app.SDLActivity\" android:screenOrientation=\"landscape\" android:exported=\"true\"><intent-filter><action android:name=\"android.intent.action.MAIN\"/><category android:name=\"android.intent.category.LAUNCHER\"/></intent-filter></activity></application></manifest>\n";
        std::ofstream note(build/"Android/README.txt");note<<"Android export staging created. Configure SDL3 Android AAR/Gradle and run Gradle assembleRelease. SDK/NDK presence has been validated.\n";
        std::cout<<"Android staging prepared: "<<(build/"Android")<<"\n";return 0;
    }
    std::ofstream cm(build/"CMakeLists.txt");cm<<"cmake_minimum_required(VERSION 3.25)\nproject("<<cfg.name<<" LANGUAGES CXX)\nset(CMAKE_CXX_STANDARD 20)\nset(DRAYVEN_BUILD_EDITOR OFF CACHE BOOL \"\" FORCE)\nset(DRAYVEN_BUILD_TESTS OFF CACHE BOOL \"\" FORCE)\nif(NOT DEFINED DRAYVEN_ENGINE_ROOT)\n  message(FATAL_ERROR \"Pass -DDRAYVEN_ENGINE_ROOT=/path/to/DrayvenEngine\")\nendif()\nadd_subdirectory(${DRAYVEN_ENGINE_ROOT} drayven-engine)\nadd_executable("<<cfg.name;
    for(auto&p:cpp)cm<<" \""<<p.generic_string()<<"\"";cm<<")\ntarget_link_libraries("<<cfg.name<<" PRIVATE DrayvenRuntime)\n";
    std::cout<<"Desktop build staging ready in "<<build<<"\nRun: cmake -S "<<q(build)<<" -B "<<q(build/"native")<<" -DDRAYVEN_ENGINE_ROOT=<engine-root> && cmake --build "<<q(build/"native")<<" --config Release\n";return 0;
}
int main(int argc,char**argv){try{if(argc<2){usage();return 0;}std::string cmd=argv[1];
    if(cmd=="new"&&argc>=4){auto kind=(argc>=5&&std::string(argv[4])=="3d")?ProjectKind::Game3D:ProjectKind::Game2D;auto c=Project::create(argv[2],argv[3],kind);std::ofstream s(c.root/"Scripts/Main.drys");s<<"game Main\nfn start()\n    log(\"Hello from DRYS\")\nend\n";std::cout<<Project::filePath(c)<<"\n";return 0;}
    if(cmd=="transpile"&&argc>=3){std::filesystem::path in=argv[2],out=in;out.replace_extension(".cpp");if(argc>=5&&std::string(argv[3])=="-o")out=argv[4];drys::Transpiler t;auto r=t.transpileFile(in);if(!r.ok){for(auto&d:r.diagnostics)std::cerr<<d<<"\n";return 2;}std::ofstream f(out);f<<r.cpp;std::cout<<out<<"\n";return 0;}
    if(cmd=="pack"&&argc>=5){PackArchive::packDirectory(argv[2],argv[3],argv[4]);std::cout<<argv[3]<<"\n";return 0;}
    if(cmd=="build"&&argc>=3){std::string target="desktop";if(argc>=5&&std::string(argv[3])=="--target")target=argv[4];return buildProject(argv[2],target);}
    usage();return 1;
}catch(const std::exception&e){std::cerr<<"drayvenc: "<<e.what()<<"\n";return 1;}}
