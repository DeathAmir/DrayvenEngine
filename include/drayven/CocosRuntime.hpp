#pragma once
#include <string>
#include <string_view>

namespace drayven {

struct RuntimeInfo {
    std::string backend;
    std::string ui;
    std::string platform;
};

class CocosRuntime {
public:
    static RuntimeInfo info();
    static bool validateDrys(std::string_view source, std::string* diagnostics = nullptr);
};

}
