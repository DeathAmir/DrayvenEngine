#include <drayven/Plugin.hpp>
using namespace drayven;
static bool load(const PluginHostApi* host) {
    if (host && host->log) host->log("Example C++ plugin loaded");
    return true;
}
static void unload() {}
DRAYVEN_PLUGIN_EXPORT const PluginDescriptor* DrayvenPluginEntry(const PluginHostApi*) {
    static PluginDescriptor p{PluginApiVersion, "ExamplePlugin", "1.0.0", &load, &unload};
    return &p;
}
