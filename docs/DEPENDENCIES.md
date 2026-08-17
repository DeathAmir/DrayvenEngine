# Dependencies

Drayven feature-gates large optional systems. SDL3 is the platform layer. RmlUi 6.2 + FreeType power the editor/UI stack. LuaJIT 2.1 is an optional script runtime/tool. Mbed TLS 3.6 provides authenticated asset packaging. Vulkan is auto-detected, Eigen and libcurl are optional, and UPX is an external optional desktop packer.

CMake switches: `DRAYVEN_BUILD_EDITOR`, `DRAYVEN_BUILD_TESTS`, `DRAYVEN_ENABLE_EIGEN`, `DRAYVEN_ENABLE_NETWORKING`, `DRAYVEN_ENABLE_MBEDTLS`, `DRAYVEN_ENABLE_VULKAN`, `DRAYVEN_BUILD_SHARED_ENGINE`.
