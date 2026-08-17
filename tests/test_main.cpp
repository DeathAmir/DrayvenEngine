#include "drayven/Drys.hpp"
#include "drayven/PackArchive.hpp"
#include "drayven/Project.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

int main(){
    using namespace drayven;
    drys::Lexer l("game X\nvar speed = 5\nfn start()\n log(\"ok\")\nend\n");auto tokens=l.run();if(tokens.size()<8)return 1;
    drys::Transpiler t;auto r=t.transpile("game X\nvar speed = 5\nfn start()\n log(\"ok\")\nend\n","X");if(!r.ok||r.cpp.find("class X")==std::string::npos||r.cpp.find("script::log")==std::string::npos)return 2;
    auto tmp=std::filesystem::temp_directory_path()/"drayven_pack_test";std::filesystem::remove_all(tmp);std::filesystem::create_directories(tmp/"assets");{std::ofstream f(tmp/"assets/a.txt");f<<"dragon";}PackArchive::packDirectory(tmp/"assets",tmp/"a.dpack","key");auto files=PackArchive::read(tmp/"a.dpack","key");if(files.size()!=1||std::string(files[0].bytes.begin(),files[0].bytes.end())!="dragon")return 3;std::filesystem::remove_all(tmp);
    std::cout<<"Drayven tests passed\n";return 0;
}
