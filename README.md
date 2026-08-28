# Unity Vulkan Driver Guard

[English](README.md) | [简体中文](README.zh-CN.md)

Unity Vulkan Driver Guard is a pre-render GPU check for standalone Unity players. It runs before Unity creates its graphics device and is intended for games that must use Vulkan and cannot offer a Direct3D fallback.

## What It Checks

- Vulkan Loader availability and physical-device enumeration.
- Physical-device API version. The default minimum is Vulkan 1.1.
- GPU vendor and device identity.
- A vendor-specific, numeric driver deny list.
- A vendor-specific recommended driver version and download URL.

If a check fails, the native dialog shows the GPU, installed driver, Vulkan version, reason, recommended version, and an **Update driver** button. There is no render-API switch. The other action is **Exit**.

The dialog follows the operating-system language for Chinese, Japanese, and Korean. Every other system language uses English.

## UE5 Reference Model

The rule and message flow follows the public UE5 RHI startup model:

- `FGPUDriverInfo`-style vendor normalization and numeric version comparison.
- `DriverDenyList` entries with `<`, `<=`, `=`, `>=`, or `>` operators.
- `SuggestedDriverVersion` and vendor download URL fields.
- Early startup blocking equivalent to UE's `RHIDetectAndWarnOfBadDrivers` path.

The project does not copy Epic source code. It implements the same data model and startup behavior for Unity's player ABI.

## Single-Executable Integration

The Unity postprocessor patches a completed player in place:

1. `UnityPlayer.dll` / `UnityPlayer.so` from the Unity build is renamed to `UnityPlayerI.dll` / `UnityPlayerI.so`.
2. The same-name proxy is copied into its place.
3. The existing game executable is unchanged and still starts one process.
4. The proxy runs the preflight, then forwards to Unity's original player entry point.

Windows exports `UnityMain` and `UnityMain2`. Unity 6 Linux exports the actual `PlayerMain(int, char**)` symbol (`_Z10PlayerMainiPPc`), which the Linux proxy forwards.

## Unity Installation

This repository is a complete Unity package. It can be copied directly into a project's `Packages/UnityVulkanDriverGuard` directory:

```text
YourGame/
  Packages/
    UnityVulkanDriverGuard/
      package.json
      Editor/
      Native~/
```

When installed this way, no files need to be copied into `Assets/Plugins`. `Native~` is package data consumed by the build postprocessor; it is intentionally not imported as a runtime Unity plugin.

Alternatively, add this repository as a local/Git package in `Packages/manifest.json`:

```json
"com.fstgame.unity-vulkan-driver-guard": "https://github.com/Binaryinject/UnityVulkanDriverGuard.git"
```

Open **Project Settings > Player > Unity Vulkan Driver Guard** to edit the minimum Vulkan version, recommended versions, vendor URLs, and deny-list rules. The postprocessor applies to `StandaloneWindows64` and `StandaloneLinux64` builds.

The native proxy binaries are expected at:

```text
Native~/Windows/x86_64/UnityPlayer.dll
Native~/Linux/x86_64/UnityPlayer.so
```

## Native Build

Windows (Visual Studio 2022 Build Tools):

```powershell
./scripts/build-windows.ps1
```

Linux (GCC/Clang and CMake):

```bash
./scripts/build-linux.sh
```

Both scripts run the version/configuration tests. GitHub Actions builds and uploads both proxy artifacts.

The Unity smoke project uses **IL2CPP**, not Mono, and forces Vulkan. This matches the production build baseline. The Editor C# assembly is still compiled by Unity's editor compiler, but no Mono player is used as a runtime validation target.

## Configuration Format

The generated `UnityVulkanDriverGuard.ini` is intentionally close to UE's INI style:

```ini
[Global]
MinimumVulkanVersion=1.1

[GPU_NVIDIA]
SuggestedDriverVersion=516.25
DownloadURL=https://www.nvidia.com/Download/index.aspx
+DriverDenyList=<516.25|Known Vulkan driver issue
```

Keep deny-list values based on your own compatibility validation and the current driver advisories from each vendor. The sample values are conservative starting points, not a claim that every older driver is broken.

## Platform Notes

- The probe dynamically loads `vulkan-1.dll` on Windows and `libvulkan.so.1` on Linux, so the Unity Editor and project do not need Vulkan SDK headers or libraries.
- Linux GUI buttons use `zenity`, then `kdialog`, and finally log the reason to stderr when no desktop dialog helper is available.
- Windows uses Task Dialog and falls back to `MessageBox`; the proxy is statically linked to the MSVC runtime.
- The proxy must be built for the same architecture as the Unity player (`x86_64` in the supplied targets).

UnityVulkanDriverGuard
