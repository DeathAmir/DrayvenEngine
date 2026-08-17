#include "drayven/Localization.hpp"
namespace drayven {
std::string Localization::tr(std::string_view key) const {
    static const std::unordered_map<std::string,std::string> fa{
        {"file","فایل"},{"edit","ویرایش"},{"view","نمایش"},{"build","بیلد"},{"project","پروژه"},
        {"new2d","پروژه دو بعدی جدید"},{"new3d","پروژه سه بعدی جدید"},{"open","باز کردن پروژه"},
        {"hierarchy","سلسله مراتب"},{"inspector","خصوصیات"},{"assets","دارایی‌ها"},{"console","کنسول"},
        {"script","ویرایشگر DRYS"},{"welcome","به Drayven Engine خوش آمدید"},{"create","ساخت پروژه"},
        {"english","English"},{"persian","فارسی"},{"desktop","Windows / Linux"},{"android","Android"}
    };
    if(m_language==Language::Persian){ auto it=fa.find(std::string(key)); if(it!=fa.end()) return it->second; }
    static const std::unordered_map<std::string,std::string> en{
        {"file","File"},{"edit","Edit"},{"view","View"},{"build","Build"},{"project","Project"},
        {"new2d","New 2D Project"},{"new3d","New 3D Project"},{"open","Open Project"},{"hierarchy","Hierarchy"},
        {"inspector","Inspector"},{"assets","Assets"},{"console","Console"},{"script","DRYS Script Editor"},
        {"welcome","Welcome to Drayven Engine"},{"create","Create Project"},{"english","English"},{"persian","فارسی"},
        {"desktop","Windows / Linux"},{"android","Android"}
    };
    auto it=en.find(std::string(key)); return it==en.end()?std::string(key):it->second;
}
}
