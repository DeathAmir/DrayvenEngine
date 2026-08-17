cmake_minimum_required(VERSION 3.25)
project(DrayvenEngine VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

option(DRAYVEN_BUILD_EDITOR "Build the Drayven Editor" ON)
option(DRAYVEN_BUILD_TESTS "Build engine tests" ON)
option(DRAYVEN_FETCH_DEPS "Fetch SDL3/Dear ImGui from upstream" ON)

include(FetchContent)

if(DRAYVEN_FETCH_DEPS)
  set(SDL_SHARED ON CACHE BOOL "" FORCE)
  set(SDL_STATIC OFF CACHE BOOL "" FORCE)
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.12
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(SDL3)

  FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
    GIT_SHALLOW TRUE)
  FetchContent_GetProperties(imgui)
  if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
  endif()

  find_package(OpenGL REQUIRED)
  add_library(drayven_imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
  target_include_directories(drayven_imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
  target_link_libraries(drayven_imgui PUBLIC SDL3::SDL3 OpenGL::GL)
endif()

add_library(DrayvenCore STATIC
  src/core/Project.cpp
  src/core/Scene.cpp
  src/core/Localization.cpp
  src/pack/PackArchive.cpp
  src/drys/Lexer.cpp
  src/drys/Transpiler.cpp)
target_include_directories(DrayvenCore PUBLIC include)
target_compile_definitions(DrayvenCore PUBLIC DRAYVEN_VERSION=\"${PROJECT_VERSION}\")

add_library(DrayvenRuntime STATIC
  src/runtime/Application.cpp
  src/runtime/Renderer2D.cpp
  src/runtime/Renderer3D.cpp)
target_include_directories(DrayvenRuntime PUBLIC include)
target_link_libraries(DrayvenRuntime PUBLIC DrayvenCore)
if(DRAYVEN_FETCH_DEPS)
  target_link_libraries(DrayvenRuntime PUBLIC SDL3::SDL3 OpenGL::GL)
endif()

add_executable(drayvenc src/cli/main.cpp)
target_link_libraries(drayvenc PRIVATE DrayvenCore)

if(DRAYVEN_BUILD_EDITOR)
  add_executable(DrayvenEditor
    src/editor/main.cpp
    src/editor/EditorApp.cpp
    src/editor/EditorPanels.cpp
    src/editor/EditorBuild.cpp
    src/editor/Persian.cpp)
  target_include_directories(DrayvenEditor PRIVATE include)
  target_link_libraries(DrayvenEditor PRIVATE DrayvenRuntime)
  if(DRAYVEN_FETCH_DEPS)
    target_link_libraries(DrayvenEditor PRIVATE drayven_imgui)
  endif()
  if(WIN32)
    add_custom_command(TARGET DrayvenEditor POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL3::SDL3-shared> $<TARGET_FILE_DIR:DrayvenEditor>)
  endif()
endif()

if(DRAYVEN_BUILD_TESTS)
  enable_testing()
  add_executable(drayven_tests tests/test_main.cpp)
  target_link_libraries(drayven_tests PRIVATE DrayvenCore)
  add_test(NAME drayven_tests COMMAND drayven_tests)
endif()

install(TARGETS DrayvenCore DrayvenRuntime drayvenc
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION bin
  ARCHIVE DESTINATION lib)
if(DRAYVEN_BUILD_EDITOR)
  install(TARGETS DrayvenEditor RUNTIME DESTINATION bin)
endif()
install(DIRECTORY include/ DESTINATION include)
install(DIRECTORY templates/ DESTINATION share/drayven/templates)
install(DIRECTORY assets/ DESTINATION share/drayven/assets)
