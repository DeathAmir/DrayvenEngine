# Architecture

Drayven Engine uses a small layered architecture:

- **DrayvenCore** — projects, scenes, DRYS compiler, localization and protected asset containers.
- **DrayvenRuntime** — SDL3 application lifecycle plus 2D/3D rendering entry points.
- **DrayvenEditor** — Dear ImGui docking editor with project creation, hierarchy, inspector, asset browser, console, script editor and exporters.
- **drayvenc** — command-line project creator, DRYS transpiler, packer and build staging tool.

The runtime intentionally keeps the public C++ API small so a generated game can link `DrayvenRuntime` while editor-only code remains out of the final executable.

The current renderer entry points are intentionally small scaffolds; future backends can implement SDL_GPU/Vulkan/OpenGL rendering without coupling engine runtime code to the editor.
