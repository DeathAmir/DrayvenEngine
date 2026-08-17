# Drayven Script (DRYS)

DRYS is a small game-oriented language that transpiles to C++ before native compilation. It is designed to keep gameplay scripts terse while still shipping native code.

```drys
game PlayerController
var speed = 5

fn start()
    log("ready")
end

fn update(dt)
    if key_down("D")
        move_x(speed * dt)
    else
        move_x(0)
    end
end
```

## Core syntax

- `game Name` names the generated C++ script class.
- `var name = expression` declares script state when used at top level and a local when used in a function.
- `fn name(args)` declares a function; blocks close with `end`.
- `if`, `else`, `while`, `return` provide control flow.
- `#` and `//` start comments.
- Numbers, strings, booleans, arithmetic, comparisons, `and`, `or`, and `not` are supported by the transpilation layer.

## Engine calls

The current standard bindings include `log`, `key_down`, `key_pressed`, `move_x`, `move_y`, `set_position`, `play_sound`, `spawn`, `destroy`, `delta_time`, and `rand`.

Generated files are written to `Build/Generated/*.cpp`. Project scripts do not need to be shipped as readable `.drys` files in release packages.

> Current note: several runtime bindings are scaffolding while the scene/input/audio systems are being connected; the lexer/transpiler itself already produces native C++ output.
