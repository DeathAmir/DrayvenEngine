#include "drayven/EditorApp.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <misc/cpp/imgui_stdlib.h>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace drayven {
static std::string shown(const Localization& l,std::string_view k){
    auto s=l.tr(k); return l.language()==Language::Persian?persianVisualOrder(s):s;
}

bool EditorApp::init(){
    if(!m_app.init("Drayven Engine",1600,900,true)) return false;
    IMGUI_CHECKVERSION(); ImGui::CreateContext(); auto& io=ImGui::GetIO();
    io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard|ImGuiConfigFlags_DockingEnable|ImGuiConfigFlags_ViewportsEnable;
    io.IniFilename="drayven_editor.ini";
    if(std::filesystem::exists("assets/fonts/Vazirmatn-Regular.ttf"))
        io.Fonts->AddFontFromFileTTF("assets/fonts/Vazirmatn-Regular.ttf",18.f);
    else io.Fonts->AddFontDefault();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL3_InitForOpenGL(m_app.window(),SDL_GL_GetCurrentContext());
    ImGui_ImplOpenGL3_Init("#version 130");
    auto& camera=m_scene.createEntity("Camera"); camera.hasCamera=true; camera.camera.primary=true;
    m_scene.createEntity("Player"); addLog("Drayven Editor ready. DRYS compiler online."); return true;
}
void EditorApp::shutdown(){ ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplSDL3_Shutdown(); ImGui::DestroyContext(); m_app.shutdown(); }
int EditorApp::run(){
    if(!init()) return 1; bool running=true;
    while(running){ SDL_Event e; while(SDL_PollEvent(&e)){ ImGui_ImplSDL3_ProcessEvent(&e); if(e.type==SDL_EVENT_QUIT||e.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED) running=false; } if(running) frame(); }
    shutdown(); return 0;
}
void EditorApp::frame(){
    ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplSDL3_NewFrame(); ImGui::NewFrame(); drawDockspace();
    if(m_showWelcome) drawWelcome(); else { drawHierarchy(); drawInspector(); drawAssets(); drawConsole(); drawScriptEditor(); if(m_showBuild) drawBuildWindow(); }
    ImGui::Render(); m_app.beginFrame(); ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    auto& io=ImGui::GetIO(); if(io.ConfigFlags&ImGuiConfigFlags_ViewportsEnable){ auto* w=SDL_GL_GetCurrentWindow(); auto c=SDL_GL_GetCurrentContext(); ImGui::UpdatePlatformWindows(); ImGui::RenderPlatformWindowsDefault(); SDL_GL_MakeCurrent(w,c); }
    m_app.endFrame();
}
void EditorApp::drawDockspace(){
    ImGui::DockSpaceOverViewport();
    if(!ImGui::BeginMainMenuBar()) return;
    if(ImGui::BeginMenu(shown(m_i18n,"file").c_str())){ if(ImGui::MenuItem(shown(m_i18n,"new2d").c_str())) m_showWelcome=true; if(ImGui::MenuItem(shown(m_i18n,"new3d").c_str())) m_showWelcome=true; if(ImGui::MenuItem("Save Scene","Ctrl+S")) saveScene(); ImGui::EndMenu(); }
    if(ImGui::BeginMenu(shown(m_i18n,"build").c_str())){ if(ImGui::MenuItem(shown(m_i18n,"desktop").c_str())) m_showBuild=true; if(ImGui::MenuItem(shown(m_i18n,"android").c_str())) m_showBuild=true; ImGui::EndMenu(); }
    if(ImGui::BeginMenu("Language")){ if(ImGui::MenuItem("English"))m_i18n.setLanguage(Language::English); if(ImGui::MenuItem(persianVisualOrder("فارسی").c_str()))m_i18n.setLanguage(Language::Persian); ImGui::EndMenu(); }
    ImGui::TextDisabled("  Drayven Engine v%s | By DeathAmir",DRAYVEN_VERSION); ImGui::EndMainMenuBar();
}
void EditorApp::drawWelcome(){
    ImGui::SetNextWindowSize({700,480},ImGuiCond_FirstUseEver); if(!ImGui::Begin("Welcome",&m_showWelcome)){ImGui::End();return;}
    ImGui::Text("%s",shown(m_i18n,"welcome").c_str()); ImGui::Separator();
    static char name[128]="MyGame"; static char path[512]="DrayvenProjects/MyGame";
    ImGui::InputText("Project Name",name,sizeof(name)); ImGui::InputText("Project Path",path,sizeof(path)); m_projectName=name; m_projectPath=path;
    if(ImGui::Button("Create 2D",{160,42})) newProject(ProjectKind::Game2D); ImGui::SameLine(); if(ImGui::Button("Create 3D",{160,42})) newProject(ProjectKind::Game3D);
    ImGui::TextWrapped("Optional Persian font: assets/fonts/Vazirmatn-Regular.ttf"); ImGui::End();
}
void EditorApp::newProject(ProjectKind kind){
    try{ m_project=Project::create(m_projectPath,m_projectName,kind); auto p=m_project->root/"Scripts/Main.drys"; std::ofstream o(p); o<<"game Main\nvar speed = 5\nfn start()\n    log(\"Drayven game started\")\nend\n"; o.close(); m_currentScript=p; std::ifstream i(p); std::stringstream ss; ss<<i.rdbuf(); m_scriptBuffer=ss.str(); m_showWelcome=false; addLog("Created project: "+m_project->root.string()); }
    catch(const std::exception& e){ addLog(std::string("Create failed: ")+e.what()); }
}
bool EditorApp::openProject(const std::filesystem::path& f){ try{m_project=Project::load(f);m_showWelcome=false;return true;}catch(const std::exception&e){addLog(e.what());return false;} }
void EditorApp::addLog(std::string t){ m_logs.push_back(std::move(t)); if(m_logs.size()>500)m_logs.erase(m_logs.begin(),m_logs.begin()+100); }
}
