cmake_minimum_required(VERSION 3.20)
project(DrayvenEngine VERSION 0.3.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

option(DRAYVEN_BUILD_STUDIO "Build the FairyGUI Drayven Studio editor" ON)
option(DRAYVEN_BUILD_TESTS "Build engine tests" ON)
option(DRAYVEN_FETCH_DEPS "Fetch Drayven utility dependencies" ON)
option(DRAYVEN_WITH_COCOS "Use Cocos2d-x v4 + FairyGUI runtime" ON)
option(DRAYVEN_ENABLE_EIGEN "Enable Eigen integration" OFF)
option(DRAYVEN_ENABLE_NETWORKING "Enable libcurl HTTP client" OFF)
option(DRAYVEN_ENABLE_MBEDTLS "Enable Mbed TLS crypto helpers" ON)
option(DRAYVEN_BUILD_SHARED_ENGINE "Build shared Drayven runtime" ON)
option(DRAYVEN_BUILD_STATIC_ENGINE "Build static Drayven runtime" ON)
set(DRAYVEN_GAME_SOURCES "" CACHE STRING "Semicolon-separated generated game sources compiled into DrayvenEngine")
set(DRAYVEN_LUAJIT_ROOT "" CACHE PATH "Optional LuaJIT prebuilt root")
set(DRAYVEN_COCOS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/vendor/cocos2d-x" CACHE PATH "Pinned Cocos2d-x source root")
set(DRAYVEN_FAIRYGUI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/vendor/FairyGUI-cocos2dx" CACHE PATH "Pinned FairyGUI-cocos2dx source root")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DrayvenDependencies)
drayven_setup_dependencies()

if(DRAYVEN_WITH_COCOS)
  if(NOT EXISTS "${DRAYVEN_COCOS_ROOT}/cocos/CMakeLists.txt")
    message(FATAL_ERROR "Cocos2d-x vendor source is missing. Run: python tools/bootstrap_vendor.py")
  endif()
  if(NOT EXISTS "${DRAYVEN_FAIRYGUI_ROOT}/libfairygui/CMakeLists.txt")
    message(FATAL_ERROR "FairyGUI vendor source is missing. Run: python tools/bootstrap_vendor.py")
  endif()

  set(COCOS2DX_ROOT_PATH "${DRAYVEN_COCOS_ROOT}")
  list(PREPEND CMAKE_MODULE_PATH "${DRAYVEN_COCOS_ROOT}/cmake/Modules")
  include(CocosBuildSet)
  set(BUILD_LUA_LIBS OFF CACHE BOOL "" FORCE)
  add_subdirectory("${DRAYVEN_COCOS_ROOT}/cocos" "${CMAKE_BINARY_DIR}/vendor/cocos/core")
  set(BUILD_ENGINE_DONE ON)
  add_subdirectory("${DRAYVEN_FAIRYGUI_ROOT}/libfairygui" "${CMAKE_BINARY_DIR}/vendor/fairygui")
endif()

set(DRAYVEN_CORE_SOURCES
  src/core/Project.cpp
  src/core/Scene.cpp
  src/core/Localization.cpp
  src/core/UiDocument.cpp
  src/core/Plugin.cpp
  src/core/Net.cpp
  src/core/Crypto.cpp
  src/pack/PackArchive.cpp
  src/drys/Lexer.cpp
  src/drys/Transpiler.cpp
  src/build/BuildPipeline.cpp)

set(DRAYVEN_RUNTIME_SOURCES
  src/runtime/CocosRuntime.cpp
  src/runtime/LuaRuntime.cpp)

add_library(DrayvenCore STATIC ${DRAYVEN_CORE_SOURCES})
target_include_directories(DrayvenCore PUBLIC include)
target_compile_definitions(DrayvenCore PUBLIC DRAYVEN_VERSION="${PROJECT_VERSION}")
if(MSVC)
  target_compile_options(DrayvenCore PRIVATE /utf-8)
else()
  target_compile_options(DrayvenCore PRIVATE -fvisibility=hidden)
endif()
if(UNIX)
  target_link_libraries(DrayvenCore PUBLIC ${CMAKE_DL_LIBS})
endif()
if(TARGET Eigen3::Eigen)
  target_link_libraries(DrayvenCore PUBLIC Eigen3::Eigen)
  target_compile_definitions(DrayvenCore PUBLIC DRAYVEN_HAS_EIGEN=1)
endif()
if(TARGET CURL::libcurl)
  target_link_libraries(DrayvenCore PRIVATE CURL::libcurl)
  target_compile_definitions(DrayvenCore PRIVATE DRAYVEN_HAS_CURL=1)
endif()
if(TARGET MbedTLS::mbedcrypto)
  target_link_libraries(DrayvenCore PRIVATE MbedTLS::mbedcrypto)
  target_compile_definitions(DrayvenCore PRIVATE DRAYVEN_HAS_MBEDTLS=1)
elseif(TARGET mbedcrypto)
  target_link_libraries(DrayvenCore PRIVATE mbedcrypto)
  target_compile_definitions(DrayvenCore PRIVATE DRAYVEN_HAS_MBEDTLS=1)
endif()

add_library(DrayvenRuntime STATIC ${DRAYVEN_RUNTIME_SOURCES})
target_include_directories(DrayvenRuntime PUBLIC include)
target_link_libraries(DrayvenRuntime PUBLIC DrayvenCore)
if(DRAYVEN_WITH_COCOS)
  target_link_libraries(DrayvenRuntime PUBLIC fairygui cocos2d)
  target_compile_definitions(DrayvenRuntime PUBLIC DRAYVEN_HAS_COCOS=1 DRAYVEN_HAS_FAIRYGUI=1)
endif()
if(MSVC)
  target_compile_options(DrayvenRuntime PRIVATE /utf-8)
endif()

if(DRAYVEN_BUILD_SHARED_ENGINE)
  add_library(DrayvenEngine SHARED src/runtime/EngineShared.cpp)
  target_include_directories(DrayvenEngine PUBLIC include)
  target_link_libraries(DrayvenEngine PRIVATE DrayvenRuntime)
  target_compile_definitions(DrayvenEngine PRIVATE DRAYVEN_ENGINE_BUILD=1)
  if(DRAYVEN_GAME_SOURCES)
    target_sources(DrayvenEngine PRIVATE ${DRAYVEN_GAME_SOURCES})
  endif()
  set_target_properties(DrayvenEngine PROPERTIES OUTPUT_NAME "DrayvenEngine")
  if(MSVC)
    target_compile_options(DrayvenEngine PRIVATE /utf-8)
  endif()
endif()

if(DRAYVEN_BUILD_STATIC_ENGINE)
  add_library(DrayvenEngineStatic STATIC
    src/runtime/EngineShared.cpp
    ${DRAYVEN_CORE_SOURCES}
    ${DRAYVEN_RUNTIME_SOURCES})
  target_include_directories(DrayvenEngineStatic PUBLIC include)
  target_compile_definitions(DrayvenEngineStatic PUBLIC DRAYVEN_VERSION="${PROJECT_VERSION}")
  if(DRAYVEN_GAME_SOURCES)
    target_sources(DrayvenEngineStatic PRIVATE ${DRAYVEN_GAME_SOURCES})
  endif()
  if(DRAYVEN_WITH_COCOS)
    target_link_libraries(DrayvenEngineStatic PUBLIC fairygui cocos2d)
    target_compile_definitions(DrayvenEngineStatic PUBLIC DRAYVEN_HAS_COCOS=1 DRAYVEN_HAS_FAIRYGUI=1)
  endif()
  if(TARGET Eigen3::Eigen)
    target_link_libraries(DrayvenEngineStatic PUBLIC Eigen3::Eigen)
    target_compile_definitions(DrayvenEngineStatic PUBLIC DRAYVEN_HAS_EIGEN=1)
  endif()
  if(TARGET CURL::libcurl)
    target_link_libraries(DrayvenEngineStatic PUBLIC CURL::libcurl)
    target_compile_definitions(DrayvenEngineStatic PUBLIC DRAYVEN_HAS_CURL=1)
  endif()
  if(TARGET MbedTLS::mbedcrypto)
    target_link_libraries(DrayvenEngineStatic PUBLIC MbedTLS::mbedcrypto)
    target_compile_definitions(DrayvenEngineStatic PUBLIC DRAYVEN_HAS_MBEDTLS=1)
  elseif(TARGET mbedcrypto)
    target_link_libraries(DrayvenEngineStatic PUBLIC mbedcrypto)
    target_compile_definitions(DrayvenEngineStatic PUBLIC DRAYVEN_HAS_MBEDTLS=1)
  endif()
  if(UNIX)
    target_link_libraries(DrayvenEngineStatic PUBLIC ${CMAKE_DL_LIBS})
  endif()
  set_target_properties(DrayvenEngineStatic PROPERTIES OUTPUT_NAME "DrayvenEngine")
  if(MSVC)
    set_target_properties(DrayvenEngineStatic PROPERTIES OUTPUT_NAME "DrayvenEngineStatic")
    target_compile_options(DrayvenEngineStatic PRIVATE /utf-8)
  else()
    target_compile_options(DrayvenEngineStatic PRIVATE -fvisibility=hidden)
  endif()
endif()

add_executable(drayvenc src/cli/main.cpp)
target_link_libraries(drayvenc PRIVATE DrayvenCore)
if(MSVC)
  target_compile_options(drayvenc PRIVATE /utf-8)
endif()

add_executable(drayvenluac src/cli/luac.cpp)
target_link_libraries(drayvenluac PRIVATE DrayvenCore)
if(MSVC)
  target_compile_options(drayvenluac PRIVATE /utf-8)
endif()

if(DRAYVEN_BUILD_STUDIO AND NOT ANDROID)
  if(NOT DRAYVEN_WITH_COCOS)
    message(FATAL_ERROR "Drayven Studio requires DRAYVEN_WITH_COCOS=ON")
  endif()
  add_executable(DrayvenEditor
    src/studio/main.cpp
    src/studio/AppDelegate.cpp
    src/studio/StudioScene.cpp)
  target_include_directories(DrayvenEditor PRIVATE
    include
    src/studio
    "${DRAYVEN_FAIRYGUI_ROOT}/libfairygui/Classes")
  target_link_libraries(DrayvenEditor PRIVATE DrayvenRuntime fairygui cocos2d)
  if(MSVC)
    target_compile_options(DrayvenEditor PRIVATE /utf-8)
  endif()
  set_target_properties(DrayvenEditor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/DrayvenEditor")
endif()

if(DRAYVEN_BUILD_TESTS)
  enable_testing()
  add_executable(drayven_tests tests/test_main.cpp)
  target_link_libraries(drayven_tests PRIVATE DrayvenCore)
  if(MSVC)
    target_compile_options(drayven_tests PRIVATE /utf-8)
  endif()
  add_test(NAME drayven_tests COMMAND drayven_tests)
endif()

install(TARGETS DrayvenCore DrayvenRuntime drayvenc drayvenluac
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION bin
  ARCHIVE DESTINATION lib)
if(DRAYVEN_BUILD_SHARED_ENGINE)
  install(TARGETS DrayvenEngine
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib)
endif()
if(DRAYVEN_BUILD_STATIC_ENGINE)
  install(TARGETS DrayvenEngineStatic ARCHIVE DESTINATION lib)
endif()
if(DRAYVEN_BUILD_STUDIO AND NOT ANDROID)
  install(TARGETS DrayvenEditor RUNTIME DESTINATION bin)
endif()
install(DIRECTORY include/ DESTINATION include)
install(DIRECTORY templates/ DESTINATION share/drayven/templates)
install(DIRECTORY assets/ DESTINATION share/drayven/assets)
install(DIRECTORY cmake/ DESTINATION share/drayven/cmake)
