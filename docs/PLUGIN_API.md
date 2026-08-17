# Plugin API

Drayven native plugins use a small C ABI so plugins can be built without exposing internal editor/runtime C++ classes.

```cpp
#include <drayven/Plugin.hpp>
using namespace drayven;
static bool on_load(const PluginHostApi* host) {
    if (host && host->log) host->log("Example plugin loaded");
    return true;
}
static void on_unload() {}
DRAYVEN_PLUGIN_EXPORT const PluginDescriptor* DrayvenPluginEntry(const PluginHostApi*) {
    static PluginDescriptor plugin{PluginApiVersion,"ExamplePlugin","1.0.0",&on_load,&on_unload};
    return &plugin;
}
```

See `templates/plugin_cpp`. Reusable DRYS modules are compiled with the final game; reusable Lua modules can become LuaJIT bytecode during release packaging.
