<div align="center">

# 🐉 Drayven Engine

### Native C++ game engine + RmlUi editor + DRYS / LuaJIT scripting

**By DeathAmir**

Windows · Linux · Android NDK · 2D · 3D · Native plugins · Encrypted asset packs

</div>

---

Drayven Engine is a C++20 game-engine foundation focused on native builds, a compact editor, script-to-native workflows and portable SDK packaging. Version 0.2 replaces the original Dear ImGui editor shell with **RmlUi**, adds a custom borderless editor chrome, adds a visual game UI creator, expands DRYS into native/Lua backends, and adds a real Android NDK shared-library pipeline.

## Highlights

- **RmlUi Editor** — CSS/RML driven editor shell instead of raw ImGui widgets.
- **Custom window chrome** — Drayven titlebar, drag/resize hit testing, custom minimize/maximize/close controls.
- **Visible green dragon branding** — editor titlebar + viewport mark + packaged SVG source.
- **UI Creator** — panel, label, button, input and progress widgets with named event handlers; saves `.rml` + `.dui.json`.
- **DRYS** — simple game-oriented language with C++ and Lua output backends.
- **Native script build** — DRYS → generated C++ → compiled into `DrayvenEngine.dll`, `libDrayvenEngine.so`, or Android `libDrayvenEngine.so`.
- **LuaJIT mode** — direct `.lua` or DRYS → Lua → LuaJIT bytecode; release packs do not need to contain the original Lua source.
- **`drayvenluac`** — Drayven wrapper around LuaJIT bytecode compilation.
- **Android NDK exporter** — explicit SDK/NDK paths, ABI and API level; CI validates ARM64.
- **Native plugin ABI** — load `.dll` / `.so` engine extensions using a small C interface.
- **`.dpack` asset integrity** — authenticated AES-256-GCM packaging when Mbed TLS is enabled; legacy v1 packs remain readable.
- **Optional native integrations** — Vulkan SDK, Eigen and libcurl can be enabled only when a project needs them.
- **UPX integration** — optional post-build compression for desktop binaries.
- **Persian + English editor** — RTL visual shaping path and optional Vazirmatn font loading. The repository intentionally does not redistribute a font binary.

## Project layout

```text
DrayvenEngine/
├─ assets/
│  ├─ editor/             RmlUi editor documents/styles
│  ├─ fonts/              optional user-provided fonts
│  └─ icons/              Drayven branding source
├─ cmake/                 dependency/build modules
├─ docs/                  scripting, Android, UI, plugin docs
├─ include/drayven/       public C++ SDK
├─ src/
│  ├─ build/              native exporter
│  ├─ cli/                drayvenc + drayvenluac
│  ├─ core/               project, UI, crypto, net, plugins
│  ├─ drys/               DRYS lexer/transpiler
│  ├─ editor/             RmlUi editor + SDL3 backend
│  ├─ pack/               .dpack archive system
│  └─ runtime/            runtime, renderers, LuaJIT loader
├─ templates/             2D/3D/plugin templates
└─ tools/                 LuaJIT helper builds
```

## Build the SDK

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Useful switches:

```text
-DDRAYVEN_BUILD_EDITOR=ON|OFF
-DDRAYVEN_BUILD_TESTS=ON|OFF
-DDRAYVEN_ENABLE_MBEDTLS=ON|OFF
-DDRAYVEN_ENABLE_VULKAN=ON|OFF
-DDRAYVEN_ENABLE_EIGEN=ON|OFF
-DDRAYVEN_ENABLE_NETWORKING=ON|OFF
```

## Create a project

```bash
drayvenc new MyGame MyGame 3d
```

Example DRYS:

```drys
game Player
var speed = 6

fn start()
    log("Player ready")
end

fn update(dt)
    if key_down("D")
        move_x(speed * dt)
    end
end
```

## DRYS → native C++

```bash
drayvenc transpile MyGame/Scripts/Player.drys --to cpp --harden -o Player.cpp
```

For a project build:

```bash
drayvenc build MyGame/MyGame.drayven --target desktop --script native --engine /path/to/DrayvenEngine
```

The generated native DRYS translation units are added directly to the shared `DrayvenEngine` target.

## LuaJIT

Direct Lua and DRYS-to-Lua are supported:

```bash
drayvenc build MyGame/MyGame.drayven --target desktop --script luajit --luajit /path/to/luajit
```

Compile one file:

```bash
drayvenluac --luajit /path/to/luajit input.lua output.dluac
```

## Android

```bash
drayvenc build MyGame/MyGame.drayven \
  --target android \
  --script native \
  --engine /path/to/DrayvenEngine \
  --sdk /path/to/Android/Sdk \
  --ndk /path/to/Android/Sdk/ndk/28.x \
  --abi arm64-v8a \
  --api 24
```

Main output:

```text
libDrayvenEngine.so
```

When using native DRYS mode, translated scripts are part of that shared library.

## Editor UI Creator

Open **UI Creator**, add components, enter handler names such as `on_play`, and save. Drayven emits:

```text
Assets/UI/Main.rml
Assets/UI/Main.dui.json
```

The runtime/editor share the same RmlUi document model, so the UI created in the editor is not a separate proprietary mockup format.

## Dependencies

Core dependencies are intentionally modular. SDL3 is the platform layer. RmlUi + FreeType are used by the editor/UI stack. Mbed TLS backs authenticated asset encryption. LuaJIT is prepared as a separately built tool/runtime. Vulkan, Eigen and libcurl are optional.

See [`docs/DEPENDENCIES.md`](docs/DEPENDENCIES.md).

## Documentation

- [`docs/SCRIPTING.md`](docs/SCRIPTING.md) — DRYS + LuaJIT dictionary
- [`docs/ANDROID.md`](docs/ANDROID.md) — SDK/NDK and `.so` export
- [`docs/UI_BUILDER.md`](docs/UI_BUILDER.md) — visual UI workflow
- [`docs/PLUGIN_API.md`](docs/PLUGIN_API.md) — native/DRYS/Lua extensions
- [`docs/DEPENDENCIES.md`](docs/DEPENDENCIES.md) — dependency matrix

## Release-code note

Generated identifiers and LuaJIT bytecode are release-format features, not a security boundary. Keep secrets and authoritative game logic on trusted services when appropriate.

## License / copyright

Copyright © DeathAmir. See [`LICENSE`](LICENSE) for this repository's license and the third-party projects' own licenses when redistributing their binaries.
