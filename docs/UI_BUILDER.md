# UI Creator

The editor and game UI share RmlUi documents. UI Creator can add Panel, Label, Button, Input and Progress widgets. Each widget has a stable id, geometry, text and an optional handler name such as `on_play`.

Saving creates `Assets/UI/Main.rml` and `Assets/UI/Main.dui.json`. The RML file is renderable markup; the sidecar stores Drayven handler metadata for scripting/runtime binding.
