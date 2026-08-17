#pragma once
#include <string>
#include <string_view>
#include <unordered_map>

namespace drayven {
enum class Language { English, Persian };
class Localization {
public:
    void setLanguage(Language lang) { m_language = lang; }
    Language language() const { return m_language; }
    std::string tr(std::string_view key) const;
private:
    Language m_language{Language::English};
};
}
