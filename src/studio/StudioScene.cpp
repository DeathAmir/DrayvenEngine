#include "StudioScene.hpp"
#include "drayven/CocosRuntime.hpp"
#include <sstream>

USING_NS_CC;

namespace {
Color4F c4(int r, int g, int b, int a = 255) {
    return Color4F(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}
}

bool StudioScene::init() {
    if (!Scene::init()) return false;

    root_ = GRoot::create(this);
    root_->retain();

    buildChrome();
    buildSceneTree();
    buildInspector();
    buildFilesystem();
    buildConsole();
    switchMode("3D");
    return true;
}

StudioScene::~StudioScene() {
    CC_SAFE_RELEASE(root_);
}

GGraph* StudioScene::panel(GComponent* parent, float x, float y, float w, float h, const Color4F& color, bool touchable) {
    auto* graph = GGraph::create();
    graph->drawRect(w, h, 0, c4(0, 0, 0, 0), color);
    graph->setPosition(x, y);
    graph->setSize(w, h);
    graph->setTouchable(touchable);
    parent->addChild(graph);
    return graph;
}

GBasicTextField* StudioScene::label(GComponent* parent, const std::string& text, float x, float y, float w, float h, float size, const Color3B& color) {
    auto* field = GBasicTextField::create();
    field->setAutoSize(AutoSizeType::NONE);
    field->setSingleLine(true);
    field->setText(text);
    field->setFontSize(size);
    field->setColor(color);
    field->setPosition(x, y);
    field->setSize(w, h);
    field->setTouchable(false);
    parent->addChild(field);
    return field;
}

GGraph* StudioScene::button(GComponent* parent, const std::string& text, float x, float y, float w, float h, const std::function<void()>& action, bool accent) {
    auto* box = panel(parent, x, y, w, h, accent ? c4(62, 149, 222) : c4(54, 59, 68), true);
    box->addClickListener([action](EventContext*) { action(); });
    label(parent, text, x + 10, y + 7, w - 20, h - 12, 14, accent ? Color3B::WHITE : Color3B(220, 224, 230));
    return box;
}

void StudioScene::buildChrome() {
    panel(root_, 0, 0, 1440, 54, c4(31, 34, 39));
    panel(root_, 0, 53, 1440, 1, c4(64, 69, 79));
    label(root_, "DRAYVEN", 18, 14, 120, 28, 20, Color3B(105, 190, 255));
    label(root_, "Studio 0.3", 132, 18, 100, 24, 13, Color3B(144, 151, 162));

    button(root_, "2D", 268, 10, 64, 34, [this] { switchMode("2D"); });
    button(root_, "3D", 338, 10, 64, 34, [this] { switchMode("3D"); }, true);
    button(root_, "Script", 408, 10, 82, 34, [this] { switchMode("Script"); });

    button(root_, "Run", 1184, 10, 72, 34, [this] { setStatus("Run requested - project launcher ready", Color3B(102, 214, 146)); }, true);
    button(root_, "Build", 1262, 10, 76, 34, [this] { setStatus("Universal build pipeline selected", Color3B(102, 214, 146)); });
    button(root_, "...", 1344, 10, 74, 34, [this] { setStatus("Editor options"); });

    center_ = GComponent::create();
    center_->setPosition(250, 54);
    center_->setSize(900, 610);
    root_->addChild(center_);

    panel(root_, 0, 854, 1440, 46, c4(31, 34, 39));
    panel(root_, 0, 854, 1440, 1, c4(64, 69, 79));
    status_ = label(root_, "Ready - Cocos2d-x + FairyGUI runtime", 18, 867, 900, 22, 13, Color3B(156, 163, 175));
    label(root_, "DRYS", 1294, 867, 50, 22, 13, Color3B(105, 190, 255));
    label(root_, "UTF-8", 1350, 867, 70, 22, 13, Color3B(130, 137, 147));
}

void StudioScene::buildSceneTree() {
    panel(root_, 0, 54, 250, 610, c4(38, 42, 48));
    panel(root_, 249, 54, 1, 610, c4(65, 70, 79));
    label(root_, "SCENE", 14, 68, 100, 24, 13, Color3B(164, 171, 181));
    button(root_, "+", 204, 65, 30, 26, [this] { setStatus("Create node"); });

    label(root_, "▼  World", 16, 108, 210, 24, 14, Color3B(224, 228, 234));
    label(root_, "    ◇  Player", 26, 140, 200, 24, 14, Color3B(198, 205, 214));
    label(root_, "    ◇  Camera", 26, 170, 200, 24, 14, Color3B(198, 205, 214));
    label(root_, "    ☀  Sun", 26, 200, 200, 24, 14, Color3B(198, 205, 214));
    label(root_, "    ▧  Environment", 26, 230, 200, 24, 14, Color3B(198, 205, 214));

    panel(root_, 10, 278, 230, 1, c4(63, 68, 77));
    label(root_, "FAVORITES", 14, 294, 120, 22, 12, Color3B(126, 133, 143));
    label(root_, "★ Main.drys", 24, 326, 190, 24, 13, Color3B(164, 198, 230));
    label(root_, "★ Player.drys", 24, 356, 190, 24, 13, Color3B(164, 198, 230));
}

void StudioScene::buildInspector() {
    panel(root_, 1150, 54, 290, 610, c4(38, 42, 48));
    panel(root_, 1150, 54, 1, 610, c4(65, 70, 79));
    label(root_, "INSPECTOR", 1166, 68, 130, 24, 13, Color3B(164, 171, 181));
    label(root_, "Player", 1166, 108, 220, 28, 18, Color3B(225, 230, 236));
    label(root_, "Node3D", 1166, 137, 180, 22, 12, Color3B(118, 174, 218));

    panel(root_, 1162, 172, 266, 1, c4(63, 68, 77));
    label(root_, "Transform", 1166, 188, 180, 22, 14, Color3B(210, 215, 222));
    label(root_, "Position", 1166, 222, 90, 22, 12, Color3B(139, 146, 156));
    label(root_, "X  0.00    Y  1.00    Z  0.00", 1166, 248, 250, 24, 12, Color3B(201, 207, 215));
    label(root_, "Rotation", 1166, 286, 90, 22, 12, Color3B(139, 146, 156));
    label(root_, "X  0.00    Y  0.00    Z  0.00", 1166, 312, 250, 24, 12, Color3B(201, 207, 215));
    label(root_, "Scale", 1166, 350, 90, 22, 12, Color3B(139, 146, 156));
    label(root_, "X  1.00    Y  1.00    Z  1.00", 1166, 376, 250, 24, 12, Color3B(201, 207, 215));

    panel(root_, 1162, 418, 266, 1, c4(63, 68, 77));
    label(root_, "Script", 1166, 434, 90, 22, 14, Color3B(210, 215, 222));
    button(root_, "Player.drys", 1166, 468, 246, 32, [this] { switchMode("Script"); });
}

void StudioScene::buildFilesystem() {
    panel(root_, 0, 664, 720, 190, c4(35, 39, 45));
    panel(root_, 0, 664, 1440, 1, c4(65, 70, 79));
    panel(root_, 719, 664, 1, 190, c4(65, 70, 79));
    label(root_, "FILESYSTEM", 14, 678, 120, 22, 12, Color3B(152, 159, 169));
    label(root_, "res://", 16, 712, 120, 24, 13, Color3B(105, 190, 255));
    label(root_, "▾ scripts", 40, 740, 180, 22, 13, Color3B(196, 202, 210));
    label(root_, "   Main.drys", 64, 768, 180, 22, 13, Color3B(190, 212, 232));
    label(root_, "   Player.drys", 64, 796, 180, 22, 13, Color3B(190, 212, 232));
    label(root_, "▸ assets", 260, 740, 160, 22, 13, Color3B(196, 202, 210));
    label(root_, "▸ scenes", 260, 768, 160, 22, 13, Color3B(196, 202, 210));
}

void StudioScene::buildConsole() {
    panel(root_, 720, 664, 720, 190, c4(32, 36, 42));
    label(root_, "OUTPUT", 736, 678, 90, 22, 12, Color3B(152, 159, 169));
    label(root_, "[Drayven] Runtime: Cocos2d-x v4", 738, 716, 640, 22, 12, Color3B(173, 180, 189));
    label(root_, "[Drayven] UI: FairyGUI-cocos2dx", 738, 742, 640, 22, 12, Color3B(173, 180, 189));
    label(root_, "[DRYS] compiler service online", 738, 768, 640, 22, 12, Color3B(110, 205, 148));
    label(root_, "Noesis adapter: external licensed SDK only", 738, 794, 640, 22, 12, Color3B(196, 168, 102));
}

void StudioScene::switchMode(const std::string& mode) {
    activeMode_ = mode;
    scriptEditor_ = nullptr;
    center_->removeChildren();
    if (mode == "Script") buildScriptEditor();
    else buildViewport();
    setStatus("Workspace: " + mode);
}

void StudioScene::buildViewport() {
    panel(center_, 0, 0, 900, 610, c4(27, 30, 35));
    panel(center_, 0, 0, 900, 42, c4(36, 40, 46));
    label(center_, activeMode_ + " VIEWPORT", 16, 10, 170, 24, 13, Color3B(171, 178, 188));
    label(center_, activeMode_ == "3D" ? "Perspective   Lit   Local" : "Canvas   Pixel Snap   Local", 610, 10, 270, 24, 12, Color3B(128, 136, 147));

    for (int x = 20; x < 900; x += 40) panel(center_, static_cast<float>(x), 42, 1, 568, c4(42, 46, 53));
    for (int y = 62; y < 610; y += 40) panel(center_, 0, static_cast<float>(y), 900, 1, c4(42, 46, 53));

    if (activeMode_ == "3D") {
        panel(center_, 448, 42, 2, 568, c4(73, 86, 100));
        panel(center_, 0, 324, 900, 2, c4(73, 86, 100));
        label(center_, "X", 844, 555, 20, 20, 13, Color3B(236, 96, 96));
        label(center_, "Y", 866, 535, 20, 20, 13, Color3B(104, 220, 132));
        label(center_, "Z", 820, 535, 20, 20, 13, Color3B(96, 158, 235));
        label(center_, "Player", 468, 286, 90, 24, 13, Color3B(229, 232, 236));
        panel(center_, 470, 316, 74, 44, c4(65, 113, 151, 170));
    } else {
        panel(center_, 449, 42, 2, 568, c4(79, 101, 121));
        panel(center_, 0, 325, 900, 2, c4(79, 101, 121));
        panel(center_, 300, 155, 300, 300, c4(45, 51, 60, 180));
        label(center_, "Canvas 1920 × 1080", 365, 286, 190, 24, 14, Color3B(196, 205, 216));
    }
}

void StudioScene::buildScriptEditor() {
    panel(center_, 0, 0, 900, 610, c4(27, 30, 35));
    panel(center_, 0, 0, 900, 42, c4(36, 40, 46));
    label(center_, "Player.drys", 16, 10, 220, 24, 13, Color3B(201, 218, 232));
    button(center_, "Validate", 692, 7, 90, 29, [this] { validateScript(); }, true);
    button(center_, "C++", 788, 7, 48, 29, [this] { setStatus("DRYS target: native C++"); });
    button(center_, "Lua", 842, 7, 48, 29, [this] { setStatus("DRYS target: LuaJIT"); });

    panel(center_, 10, 52, 880, 548, c4(22, 25, 30));
    panel(center_, 10, 52, 48, 548, c4(30, 34, 40));
    for (int i = 1; i <= 19; ++i) {
        label(center_, std::to_string(i), 18, 62.0f + (i - 1) * 26.0f, 30, 22, 12, Color3B(93, 101, 112));
    }

    scriptEditor_ = GTextInput::create();
    scriptEditor_->setPosition(66, 60);
    scriptEditor_->setSize(814, 528);
    scriptEditor_->setSingleLine(false);
    scriptEditor_->setFontSize(15);
    scriptEditor_->setColor(Color3B(214, 220, 229));
    scriptEditor_->setText(
        "game PlayerController\n\n"
        "var speed = 6.0\n"
        "var boost = 1.75\n"
        "var energy = 100\n\n"
        "fn update(dt)\n"
        "    var x = 0\n"
        "    var y = 0\n"
        "    if key_down(\"W\")\n"
        "        y = y - 1\n"
        "    end\n"
        "    if key_down(\"D\")\n"
        "        x = x + 1\n"
        "    end\n"
        "    move_x(x * speed * dt)\n"
        "    move_y(y * speed * dt)\n"
        "end\n");
    center_->addChild(scriptEditor_);
}

void StudioScene::setStatus(const std::string& text, const Color3B& color) {
    if (!status_) return;
    status_->setText(text);
    status_->setColor(color);
}

void StudioScene::validateScript() {
    if (!scriptEditor_) return;
    std::string diagnostics;
    if (drayven::CocosRuntime::validateDrys(scriptEditor_->getText(), &diagnostics)) {
        setStatus("DRYS validation passed", Color3B(100, 214, 143));
    } else {
        setStatus(diagnostics.empty() ? "DRYS validation failed" : diagnostics, Color3B(231, 105, 105));
    }
}
