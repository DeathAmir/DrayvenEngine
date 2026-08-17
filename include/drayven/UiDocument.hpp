#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
namespace drayven { enum class UiWidgetType { Panel,Label,Button,Input,Progress }; struct UiWidget { std::uint32_t id{}; UiWidgetType type{UiWidgetType::Label}; std::string text{"Widget"},handler; int x{24},y{24},width{180},height{42}; }; class UiDocument { public: UiWidget& add(UiWidgetType,std::string text={},std::string handler={}); bool remove(std::uint32_t); void clear(); const std::vector<UiWidget>& widgets() const{return m_widgets;} std::string toRmlFragment() const; std::string toRmlDocument(std::string_view title="Drayven UI") const; std::string handlerManifest() const; void save(const std::filesystem::path&) const; private: std::vector<UiWidget> m_widgets; std::uint32_t m_nextId{1}; }; }
