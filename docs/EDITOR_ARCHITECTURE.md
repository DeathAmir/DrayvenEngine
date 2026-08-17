# Drayven Studio 0.3 architecture

Drayven Studio 0.3 moves the editor UI away from Nuklear/RmlUi/ImGui and onto a Cocos2d-x + FairyGUI stack.

- Cocos2d-x v4 provides the cross-platform application, renderer, input, scene graph and platform layer.
- FairyGUI-cocos2dx provides the retained-mode editor UI runtime.
- DRYS remains Drayven's scripting language and compiler.
- NoesisGUI is not vendored or redistributed because its SDK is proprietary. A future optional adapter may load a user-supplied licensed Noesis SDK.
- Third-party notices are kept in `THIRD_PARTY_NOTICES.md`.

Pinned upstream revisions are recorded in `cmake/DrayvenVendor.cmake` and fetched by the CI bootstrap.
