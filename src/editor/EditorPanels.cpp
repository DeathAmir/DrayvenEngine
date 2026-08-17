#include "drayven/EditorApp.hpp"
#include <imgui.h>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace drayven {
void EditorApp::drawHierarchy(){
    if(!ImGui::Begin("Hierarchy")){ImGui::End();return;} if(ImGui::Button("+ Entity")){auto&e=m_scene.createEntity("Entity");m_selected=e.id;} ImGui::Separator();
    for(auto&e:m_scene.entities()) if(ImGui::Selectable(e.name.c_str(),e.id==m_selected))m_selected=e.id; ImGui::End();
}
void EditorApp::drawInspector(){
    if(!ImGui::Begin("Inspector")){ImGui::End();return;} auto*e=m_scene.find(m_selected); if(!e){ImGui::TextDisabled("Select an entity");ImGui::End();return;}
    char name[128];std::snprintf(name,sizeof(name),"%s",e->name.c_str());if(ImGui::InputText("Name",name,sizeof(name)))e->name=name;
    ImGui::DragFloat3("Position",&e->transform.position.x,.05f);ImGui::DragFloat3("Rotation",&e->transform.rotation.x,.25f);ImGui::DragFloat3("Scale",&e->transform.scale.x,.05f);
    ImGui::SeparatorText("Components"); ImGui::Checkbox("Sprite 2D",&e->hasSprite);ImGui::Checkbox("Mesh 3D",&e->hasMesh);ImGui::Checkbox("Camera",&e->hasCamera);ImGui::Checkbox("Audio Source",&e->hasAudio);ImGui::Checkbox("DRYS Script",&e->hasScript);ImGui::End();
}
void EditorApp::drawAssets(){
    if(!ImGui::Begin("Assets")){ImGui::End();return;} if(!m_project){ImGui::TextDisabled("No project");ImGui::End();return;} auto root=m_project->root;
    for(auto dir:{"Assets","Scripts","Scenes"}){auto p=root/dir;if(ImGui::TreeNode(dir)){if(std::filesystem::exists(p))for(auto&e:std::filesystem::recursive_directory_iterator(p))if(e.is_regular_file()){auto rel=std::filesystem::relative(e.path(),root).generic_string();if(ImGui::Selectable(rel.c_str())&&e.path().extension()==".drys"){std::ifstream in(e.path());std::stringstream ss;ss<<in.rdbuf();m_scriptBuffer=ss.str();m_currentScript=e.path();}}ImGui::TreePop();}}
    ImGui::End();
}
void EditorApp::drawConsole(){
    if(!ImGui::Begin("Console")){ImGui::End();return;}if(ImGui::Button("Clear"))m_logs.clear();ImGui::Separator();for(auto&s:m_logs)ImGui::TextWrapped("%s",s.c_str());ImGui::End();
}
}
