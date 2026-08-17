#include "drayven/Application.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <iostream>

namespace drayven {
Application::Application()=default;
Application::~Application(){ shutdown(); }
bool Application::init(const std::string& title,int width,int height,bool resizable){
    if(!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_GAMEPAD)){ std::cerr<<"SDL_Init failed: "<<SDL_GetError()<<"\n"; return false; }
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1); SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24); SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,8);
    SDL_WindowFlags flags=SDL_WINDOW_OPENGL|SDL_WINDOW_HIGH_PIXEL_DENSITY; if(resizable)flags|=SDL_WINDOW_RESIZABLE;
    m_window=SDL_CreateWindow(title.c_str(),width,height,flags); if(!m_window){std::cerr<<SDL_GetError()<<"\n";SDL_Quit();return false;}
    m_gl=SDL_GL_CreateContext(m_window); if(!m_gl){std::cerr<<SDL_GetError()<<"\n";SDL_DestroyWindow(m_window);m_window=nullptr;SDL_Quit();return false;}
    SDL_GL_MakeCurrent(m_window,(SDL_GLContext)m_gl); SDL_GL_SetSwapInterval(1); m_running=true; m_lastTicks=SDL_GetTicks(); return true;
}
void Application::shutdown(){ if(m_gl){SDL_GL_DestroyContext((SDL_GLContext)m_gl);m_gl=nullptr;} if(m_window){SDL_DestroyWindow(m_window);m_window=nullptr;} if(SDL_WasInit(0))SDL_Quit();m_running=false; }
bool Application::poll(){ SDL_Event e; while(SDL_PollEvent(&e)){ if(e.type==SDL_EVENT_QUIT)m_running=false; if(e.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED)m_running=false; } auto now=SDL_GetTicks();m_delta=float(now-m_lastTicks)/1000.f;m_lastTicks=now;return m_running; }
void Application::beginFrame(float r,float g,float b,float a){ int w=0,h=0;SDL_GetWindowSizeInPixels(m_window,&w,&h);glViewport(0,0,w,h);glEnable(GL_DEPTH_TEST);glClearColor(r,g,b,a);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT); }
void Application::endFrame(){ SDL_GL_SwapWindow(m_window); }
}
