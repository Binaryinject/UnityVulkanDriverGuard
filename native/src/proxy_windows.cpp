#include "uvdg/preflight.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <filesystem>
#include <string>

namespace {

using UnityMainFunction = int (WINAPI*)(HINSTANCE, HINSTANCE, wchar_t*, int);

int ForwardUnityMain(const char* exportName, HINSTANCE instance, HINSTANCE previous,
                     wchar_t* commandLine, int showCommand) {
    const std::filesystem::path directory = std::filesystem::u8path(uvdg::ExecutableDirectory());
    const auto configPath = (directory / "DriverGuard.ini").u8string();
    const uvdg::PreflightResult preflight = uvdg::RunPreflight(configPath);
    if (!preflight.Passed()) {
        uvdg::ShowFailureDialog(preflight);
        return 1;
    }

    const auto originalPlayerPath = directory / L"UnityPlayerI.dll";
    const HMODULE originalPlayer = LoadLibraryExW(originalPlayerPath.c_str(), nullptr,
                                                   LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!originalPlayer) {
        MessageBoxW(nullptr,
                    L"UnityPlayerI.dll could not be loaded. Reinstall the game.",
                    L"Game installation is damaged", MB_OK | MB_ICONERROR);
        return 1;
    }

    const auto originalMain = reinterpret_cast<UnityMainFunction>(
        GetProcAddress(originalPlayer, exportName));
    if (!originalMain) {
        MessageBoxW(nullptr, L"The original UnityPlayer entry point is missing.",
                    L"Game installation is damaged", MB_OK | MB_ICONERROR);
        return 1;
    }
    return originalMain(instance, previous, commandLine, showCommand);
}

}  // namespace

extern "C" __declspec(dllexport) int WINAPI UnityMain2(
    HINSTANCE instance, HINSTANCE previous, wchar_t* commandLine, int showCommand) {
    return ForwardUnityMain("UnityMain2", instance, previous, commandLine, showCommand);
}

extern "C" __declspec(dllexport) int WINAPI UnityMain(
    HINSTANCE instance, HINSTANCE previous, wchar_t* commandLine, int showCommand) {
    return ForwardUnityMain("UnityMain", instance, previous, commandLine, showCommand);
}
