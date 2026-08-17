#pragma once
#if defined(_WIN32)
 #if defined(DRAYVEN_ENGINE_BUILD)
  #define DRAYVEN_API __declspec(dllexport)
 #else
  #define DRAYVEN_API __declspec(dllimport)
 #endif
#else
 #define DRAYVEN_API __attribute__((visibility("default")))
#endif
extern "C" { DRAYVEN_API const char* Drayven_GetVersion(); DRAYVEN_API int Drayven_GetAbiVersion(); }
