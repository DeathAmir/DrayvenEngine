#include "drayven/CocosRuntime.hpp"
#include "drayven/Drys.hpp"

namespace drayven {

RuntimeInfo CocosRuntime::info() {
    RuntimeInfo out;
    out.backend = "Cocos2d-x v4";
    out.ui = "FairyGUI-cocos2dx";
#if defined(_WIN32)
    out.platform = "Windows";
#elif defined(__ANDROID__)
    out.platform = "Android";
#elif defined(__linux__)
    out.platform = "Linux";
#elif defined(__APPLE__)
    out.platform = "Apple";
#else
    out.platform = "Unknown";
#endif
    return out;
}

bool CocosRuntime::validateDrys(std::string_view source, std::string* diagnostics) {
    drys::Transpiler transpiler;
    drys::TranspileOptions options;
    options.language = drys::OutputLanguage::Cpp;
    options.hardenSymbols = false;
    options.stripComments = false;
    auto result = transpiler.transpile(source, "EditorValidation", options);
    if (diagnostics) {
        diagnostics->clear();
        for (const auto& item : result.diagnostics) {
            if (!diagnostics->empty()) diagnostics->append("\n");
            diagnostics->append(item);
        }
    }
    return result.ok;
}

}
