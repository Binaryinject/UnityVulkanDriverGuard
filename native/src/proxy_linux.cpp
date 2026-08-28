#include "uvdg/preflight.h"

#include <dlfcn.h>

#include <cstdio>
#include <string>

namespace {

using PlayerMainFunction = int (*)(int, char**);

}  // namespace

__attribute__((visibility("default"))) int PlayerMain(int argc, char** argv) {
    const std::string directory = uvdg::ExecutableDirectory();
    const auto preflight = uvdg::RunPreflight(directory + "/DriverGuard.ini");
    if (!preflight.Passed()) {
        if (!uvdg::ShowFailureDialog(preflight)) return 1;
    }

    const std::string originalPlayerPath = directory + "/UnityPlayerI.so";
    void* originalPlayer = dlopen(originalPlayerPath.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (!originalPlayer) {
        std::fprintf(stderr, "Unable to load %s: %s\n", originalPlayerPath.c_str(), dlerror());
        return 1;
    }
    const auto originalMain = reinterpret_cast<PlayerMainFunction>(
        dlsym(originalPlayer, "_Z10PlayerMainiPPc"));
    if (!originalMain) {
        std::fprintf(stderr, "UnityPlayerI.so does not export PlayerMain(int, char**).\n");
        return 1;
    }
    return originalMain(argc, argv);
}
