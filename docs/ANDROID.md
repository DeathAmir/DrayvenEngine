# Android / NDK

Drayven Android exports are native C++ builds. The main artifact is `Build/Android/<abi>/libDrayvenEngine.so`. DRYS scripts built in `native` mode are transpiled to C++ and compiled into this `.so`.

Requirements: Android SDK, Android NDK and CMake/Ninja. JDK/Gradle are needed only when wrapping the native library into an APK/AAB.

```bash
drayvenc build MyGame.drayven --target android --engine /path/to/DrayvenEngine --sdk /opt/android-sdk --ndk /opt/android-sdk/ndk/28.x --abi arm64-v8a --api 24 --script native
```

If paths are omitted, Drayven checks `ANDROID_SDK_ROOT`, `ANDROID_HOME`, `ANDROID_NDK_HOME`, and the SDK `ndk/` directory. The editor exposes explicit SDK and NDK fields. CI validates `arm64-v8a` and packages its LuaJIT Android shared library beside `libDrayvenEngine.so`.
