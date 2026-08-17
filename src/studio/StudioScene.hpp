#pragma once
#include "cocos2d.h"
#include "FairyGUI.h"
#include <functional>
#include <string>

USING_NS_FGUI;

class StudioScene final : public cocos2d::Scene {
public:
    CREATE_FUNC(StudioScene);
    bool init() override;
    ~StudioScene() override;

private:
    GRoot* root_{nullptr};
    GComponent* center_{nullptr};
    GTextInput* scriptEditor_{nullptr};
    GBasicTextField* status_{nullptr};
    std::string activeMode_{"3D"};

    GGraph* panel(GComponent* parent, float x, float y, float w, float h, const cocos2d::Color4F& color, bool touchable = false);
    GBasicTextField* label(GComponent* parent, const std::string& text, float x, float y, float w, float h, float size, const cocos2d::Color3B& color);
    GGraph* button(GComponent* parent, const std::string& text, float x, float y, float w, float h, const std::function<void()>& action, bool accent = false);
    void buildChrome();
    void buildSceneTree();
    void buildInspector();
    void buildFilesystem();
    void buildConsole();
    void switchMode(const std::string& mode);
    void buildViewport();
    void buildScriptEditor();
    void setStatus(const std::string& text, const cocos2d::Color3B& color = cocos2d::Color3B(156, 163, 175));
    void validateScript();
};
