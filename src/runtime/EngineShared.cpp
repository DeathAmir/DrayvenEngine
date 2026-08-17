#include "drayven/Engine.hpp"

#ifndef DRAYVEN_VERSION
#define DRAYVEN_VERSION "0.2.0"
#endif

extern "C" {
const char* Drayven_GetVersion() { return DRAYVEN_VERSION; }
int Drayven_GetAbiVersion() { return 2; }
}
