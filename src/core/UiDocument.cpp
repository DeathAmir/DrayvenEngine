#include "drayven/UiDocument.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace drayven {
namespace {
std::string esc(std::string_view s) {
    std::string out;
    out.reserve(s.size() + 16);
    for (char c : s) {
        switch (c) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        default: out.push_back(c); break;
        }
    }
    return out;
}
const char* typeName(UiWidgetType t) {
    switch (t) {
    case UiWidgetType::Panel: return "panel";
    case UiWidgetType::Label: return "label";
    case UiWidgetType::Button: return "button";
    case UiWidgetType::Input: return "input";
    case UiWidgetType::Progress: return "progress";
    }
    return "widget";
}
}

UiWidget& UiDocument::add(UiWidgetType type, std::string text, std::string handler) {
    UiWidget w;
    w.id = m_nextId++;
    w.type = type;
    w.text = text.empty() ? typeName(type) : std::move(text);
    w.handler = std::move(handler);
    w.x = 24 + static_cast<int>((w.id - 1) % 3) * 22;
    w.y = 24 + static_cast<int>((w.id - 1) % 8) * 54;
    if (type == UiWidgetType::Panel) { w.width = 360; w.height = 180; }
    if (type == UiWidgetType::Progress) { w.width = 260; w.height = 24; }
    m_widgets.push_back(std::move(w));
    return m_widgets.back();
}

bool UiDocument::remove(std::uint32_t id) {
    auto it = std::remove_if(m_widgets.begin(), m_widgets.end(), [id](const UiWidget& w){ return w.id == id; });
    if (it == m_widgets.end()) return false;
    m_widgets.erase(it, m_widgets.end());
    return true;
}

void UiDocument::clear() {
    m_widgets.clear();
    m_nextId = 1;
}

std::string UiDocument::toRmlFragment() const {
    std::ostringstream out;
    for (const auto& w : m_widgets) {
        const auto id = "uiw_" + std::to_string(w.id);
        out << "<div id=\"" << id << "\" class=\"uiw " << typeName(w.type)
            << "\" style=\"left:" << w.x << "px; top:" << w.y << "px; width:" << w.width
            << "px; height:" << w.height << "px;\"";
        if (!w.handler.empty()) out << " data-handler=\"" << esc(w.handler) << "\"";
        out << ">";
        switch (w.type) {
        case UiWidgetType::Panel: out << "<span class=\"uiw-title\">" << esc(w.text) << "</span>"; break;
        case UiWidgetType::Label: out << esc(w.text); break;
        case UiWidgetType::Button: out << "<button class=\"preview-button\">" << esc(w.text) << "</button>"; break;
        case UiWidgetType::Input: out << "<input type=\"text\" value=\"" << esc(w.text) << "\" />"; break;
        case UiWidgetType::Progress: out << "<div class=\"progress-track\"><div class=\"progress-fill\"></div></div>"; break;
        }
        out << "</div>\n";
    }
    if (m_widgets.empty()) out << "<div class=\"ui-empty\">Add a widget from the toolbar</div>";
    return out.str();
}

std::string UiDocument::toRmlDocument(std::string_view title) const {
    std::ostringstream out;
    out << "<rml><head><title>" << esc(title) << "</title><style>\n"
        << "body{margin:0;background:#101317;color:#e6edf3;font-family:Vazirmatn,Segoe UI,DejaVu Sans;}"
        << ".canvas{position:relative;width:100%;height:100%;overflow:hidden;}"
        << ".uiw{position:absolute;box-sizing:border-box;}"
        << ".panel{background:#171d23;border:1px #2b3540;border-radius:10px;padding:12px;}"
        << ".label{color:#eaf2f8;padding:8px;}"
        << ".button button,.preview-button{width:100%;height:100%;background:#21b66f;color:white;border:0;border-radius:8px;}"
        << ".input input{width:100%;height:100%;background:#0d1117;color:#eaf2f8;border:1px #303b46;border-radius:8px;padding:8px;}"
        << ".progress-track{width:100%;height:100%;background:#232a31;border-radius:12px;}"
        << ".progress-fill{width:62%;height:100%;background:#22c875;border-radius:12px;}"
        << "</style></head><body><div class=\"canvas\">" << toRmlFragment()
        << "</div></body></rml>";
    return out.str();
}

std::string UiDocument::handlerManifest() const {
    std::ostringstream out;
    out << "{\n  \"version\": 1,\n  \"handlers\": [\n";
    bool first = true;
    for (const auto& w : m_widgets) {
        if (w.handler.empty()) continue;
        if (!first) out << ",\n";
        first = false;
        out << "    {\"id\":\"uiw_" << w.id << "\",\"event\":\"click\",\"handler\":\"" << esc(w.handler) << "\"}";
    }
    out << "\n  ]\n}\n";
    return out.str();
}

void UiDocument::save(const std::filesystem::path& rmlFile) const {
    std::filesystem::create_directories(rmlFile.parent_path());
    std::ofstream rml(rmlFile, std::ios::binary);
    if (!rml) throw std::runtime_error("cannot write UI document: " + rmlFile.string());
    rml << toRmlDocument(rmlFile.stem().string());
    auto manifest = rmlFile;
    manifest.replace_extension(".dui.json");
    std::ofstream meta(manifest, std::ios::binary);
    if (!meta) throw std::runtime_error("cannot write UI handler manifest: " + manifest.string());
    meta << handlerManifest();
}
}
