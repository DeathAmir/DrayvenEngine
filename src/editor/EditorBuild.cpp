#include "drayven/EditorApp.hpp"
#include "drayven/Drys.hpp"
#include "drayven/PackArchive.hpp"
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <cstdlib>
#include <fstream>

namespace drayven {
void EditorApp::drawScriptEditor(){
    if(!ImGui::Begin("DRYS Script Editor")){ImGui::End();return;} if(m_currentScript.empty()){ImGui::TextDisabled("Select a .drys file");ImGui::End();return;}
    ImGui::Text("%s",m_currentScript.string().c_str()); if(ImGui::Button("Save + Transpile")){std::ofstream o(m_currentScript);o<<m_scriptBuffer;o.close();drys::Transpiler t;auto r=t.transpile(m_scriptBuffer,m_currentScript.stem().string());if(r.ok){auto gen=m_project->root/"Build/Generated";std::filesystem::create_directories(gen);std::ofstream c(gen/(m_currentScript.stem().string()+".cpp"));c<<r.cpp;addLog("DRYS transpiled to native C++.");}else for(auto&d:r.diagnostics)addLog(d);}
    ImGui::InputTextMultiline("##drys",&m_scriptBuffer,{ImGui::GetContentRegionAvail().x,ImGui::GetContentRegionAvail().y*.72f},ImGuiInputTextFlags_AllowTabInput);
    ImGui::SeparatorText("Syntax highlight preview");drys::Lexer lex(m_scriptBuffer);for(auto&t:lex.run()){if(t.kind==drys::TokenKind::End)break;if(t.kind==drys::TokenKind::Newline){ImGui::NewLine();continue;}ImVec4 col={.8f,.8f,.8f,1};if(t.kind==drys::TokenKind::Keyword)col={.9f,.45f,.3f,1};else if(t.kind==drys::TokenKind::String)col={.4f,.8f,.45f,1};else if(t.kind==drys::TokenKind::Number)col={.55f,.7f,1,1};ImGui::TextColored(col,"%s",t.text.c_str());ImGui::SameLine(0,5);}ImGui::End();
}
void EditorApp::drawBuildWindow(){
    if(!ImGui::Begin("Build & Export",&m_showBuild)){ImGui::End();return;} if(!m_project){ImGui::TextDisabled("Create/open a project first.");ImGui::End();return;}
    if(ImGui::Button("Build Windows / Linux"))buildDesktop(); if(ImGui::Button("Build Android (SDK + NDK required)"))buildAndroid(); ImGui::TextWrapped("DRYS -> C++, Assets -> GameAssets.dpack"); ImGui::End();
}
void EditorApp::buildDesktop(){
    try{auto gen=m_project->root/"Build/Generated";std::filesystem::create_directories(gen);drys::Transpiler tr;for(auto&e:std::filesystem::recursive_directory_iterator(m_project->root/"Scripts"))if(e.is_regular_file()&&e.path().extension()==".drys"){auto r=tr.transpileFile(e.path());if(!r.ok){for(auto&d:r.diagnostics)addLog(d);return;}std::ofstream o(gen/(e.path().stem().string()+".cpp"));o<<r.cpp;}PackArchive::packDirectory(m_project->root/"Assets",m_project->root/"Build/GameAssets.dpack",m_project->assetKey);addLog("Desktop staging ready in Build/.");}catch(const std::exception&e){addLog(e.what());}
}
void EditorApp::buildAndroid(){
    auto sdk=std::getenv("ANDROID_SDK_ROOT");if(!sdk)sdk=std::getenv("ANDROID_HOME");if(!sdk){addLog("Android SDK missing: set ANDROID_SDK_ROOT/ANDROID_HOME and install NDK + CMake.");return;}if(!std::filesystem::exists(std::filesystem::path(sdk)/"ndk")){addLog("Android NDK not found in SDK/ndk.");return;}buildDesktop();addLog("Android prerequisites detected; run drayvenc build <project> --target android.");
}
void EditorApp::saveScene(){
    if(!m_project)return;std::ofstream out(m_project->root/m_project->startupScene);out<<"{\n  \"entities\": [\n";for(size_t i=0;i<m_scene.entities().size();++i){auto&e=m_scene.entities()[i];out<<"    {\"id\":"<<e.id<<",\"name\":\""<<e.name<<"\"}"<<(i+1<m_scene.entities().size()?",":"")<<"\n";}out<<"  ]\n}\n";addLog("Scene saved.");
}
}
