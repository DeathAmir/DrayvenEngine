# Drayven Scripting Guide

Drayven supports three scripting paths: **DRYS → native C++**, **DRYS → Lua → LuaJIT bytecode**, and **direct Lua → LuaJIT bytecode**.

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

Core syntax includes `game`, `var`, `let`, `fn`, `if`, `else`, `while`, `return`, booleans, comments, numbers and strings. Built-ins include `log`, `key_down`, `key_pressed`, `move_x`, `move_y`, `set_position`, `play_sound`, `spawn`, `destroy`, `delta_time`, and `rand`.

Native compilation:

```bash
drayvenc transpile Scripts/Player.drys --to cpp --harden -o Build/Generated/Player.cpp
```

With the native backend, `.drys` files become C++ translation units in the `DrayvenEngine` shared target, including Android `libDrayvenEngine.so`. `--harden` replaces generated symbol names with deterministic non-semantic identifiers; it is a release-code feature, not a security boundary.

Lua backend:

```bash
drayvenc transpile Scripts/Player.drys --to lua -o Build/Lua/Player.lua
drayvenluac --luajit /path/to/luajit Build/Lua/Player.lua Build/Lua/Player.luac
```

Direct `.lua` files can be compiled with LuaJIT `-b`. C++ extensions use `PLUGIN_API.md`; DRYS and Lua modules can be packaged with projects.
