# Unity Vulkan Driver Guard

[English](README.md) | [简体中文](README.zh-CN.md)

Unity Vulkan Driver Guard 是用于 Unity PC 独立播放器的渲染前 GPU 检测工具。它会在 Unity 创建图形设备之前运行，适合必须使用 Vulkan、不能提供 Direct3D 后备方案的游戏。

## 检测内容

- Vulkan Loader 是否可用，以及能否枚举物理设备。
- 物理设备公开的 Vulkan API 版本，默认最低要求为 Vulkan 1.1。
- GPU 厂商和设备标识。
- 参考 UE5 的驱动拒绝规则，并支持 Vulkan RHI、显卡名称正则、`DeviceId` 和 `DriverId` 精确约束。
- 各厂商的建议驱动版本和官方下载地址。

检测失败时，原生弹窗会显示 GPU、已安装驱动、Vulkan 版本、失败原因、建议版本以及**更新驱动**按钮。命中驱动拒绝规则、但仍支持 Vulkan 1.1 或更高版本时，还会提供**仍然继续**按钮。缺少 Vulkan 支持或低于 Vulkan 1.1 时仍会阻止启动。弹窗不会提供切换渲染 API 的选项。

弹窗会根据操作系统语言自动选择中文、日文或韩文；除中日韩以外的所有系统语言统一使用英文。

## UE5 参考模型

规则和提示流程参考 UE5 公开的 RHI 启动模型：

- 类似 `FGPUDriverInfo` 的厂商版本归一化和数字版本比较。
- 支持 `<`、`<=`、`=`、`>=`、`>` 运算符的 `DriverDenyList` 条目。
- 只有 `RHIName`、`AdapterNameRegex`、`DeviceId`、Vulkan `DriverId` 等选择器匹配后，规则才会拦截驱动。
- 默认不设置厂商全局最低驱动版本；没有命中规则的显卡只检查 Vulkan 1.1 能力。
- `SuggestedDriverVersion` 和厂商驱动下载地址。
- 类似 UE `RHIDetectAndWarnOfBadDrivers` 流程的启动早期阻断。

本项目不复制 Epic 源代码，而是针对 Unity Player ABI 实现相同的数据模型和启动行为。

## 单一游戏可执行文件

Unity 构建后处理会直接修改已完成的 Player：

1. 将 Unity 构建生成的 `UnityPlayer.dll` / `UnityPlayer.so` 改名为 `UnityPlayerI.dll` / `UnityPlayerI.so`。
2. 将同名代理放到原来的 `UnityPlayer.dll` / `UnityPlayer.so` 位置。
3. 游戏主可执行文件保持不变，启动时仍然只有一个游戏进程。
4. 代理先执行检测，通过后再转发到 Unity 原始 Player 入口。

Windows 代理导出 `UnityMain` 和 `UnityMain2`。Unity 6 Linux 实际导出 `PlayerMain(int, char**)` 符号（`_Z10PlayerMainiPPc`），Linux 代理会转发该入口。

## Unity 安装

本仓库本身就是完整的 Unity Package，可以直接复制到项目的 `Packages/UnityVulkanDriverGuard`：

```text
YourGame/
  Packages/
    UnityVulkanDriverGuard/
      package.json
      Editor/
      Native~/
```

这种安装方式不需要向 `Assets/Plugins` 复制任何文件。`Native~` 是供构建后处理读取的包数据，不会作为普通 Unity Runtime Plugin 导入。

也可以在 `Packages/manifest.json` 中使用 Git 地址：

```json
"com.fstgame.unity-vulkan-driver-guard": "https://github.com/Binaryinject/UnityVulkanDriverGuard.git"
```

带 Tag 的 GitHub Release 会同时提供两种可直接安装的压缩包：

- 在 Unity 中通过 **Assets > Import Package > Custom Package** 导入 `.unitypackage`，它会作为嵌入式包安装到 `Packages/UnityVulkanDriverGuard`。
- 在 **Package Manager > Add package from tarball** 中选择 `.tgz`，包会继续位于 `Packages` 下。

Release 流程会先编译并测试 Windows、Linux 两个平台的原生代理，再生成这两种压缩包。`.unitypackage` 自带全部内容，用户不需要额外安装导入工具；两种安装方式的构建行为相同。

在 **Project Settings > Player > Unity Vulkan Driver Guard** 中可以编辑最低 Vulkan 版本、建议驱动版本、厂商下载地址和拒绝列表。构建后处理适用于 `StandaloneWindows64` 和 `StandaloneLinux64`。

原生代理文件应位于：

```text
Native~/Windows/x86_64/UnityPlayer.dll
Native~/Linux/x86_64/UnityPlayer.so
```

## 原生构建

Windows，需要 Visual Studio 2022 Build Tools：

```powershell
./scripts/build-windows.ps1
```

Linux，需要 GCC/Clang 和 CMake：

```bash
./scripts/build-linux.sh
```

两个脚本都会运行版本和配置规则测试。GitHub Actions 会构建并上传两个平台的代理产物。

Unity 冒烟工程强制使用 **IL2CPP** 和 Vulkan，不使用 Mono Player，保持与正式包的构建基线一致。Editor C# 程序集仍由 Unity 编辑器编译器完成编译。

## 配置格式

生成的 `DriverGuard.ini` 特意采用接近 UE 的 INI 规则格式：

```ini
[Global]
MinimumVulkanVersion=1.1

[GPU_NVIDIA]
SuggestedDriverVersion=516.25
DownloadURL=https://www.nvidia.com/Download/index.aspx
+DriverDenyList=(DriverVersion="<516.25",RHIName="Vulkan",DeviceId="0x1B80-0x1B8F",DriverId="NVIDIA_PROPRIETARY",Reason="Known Vulkan driver issue")
```

拒绝列表应基于项目自己的兼容性验证结果以及显卡厂商当前的驱动公告进行维护。示例版本只是保守的初始值，并不代表所有更早版本都一定存在问题。

### 使用 pci.ids 自动生成 Device ID

可以在构建或发布阶段使用 `pci.ids` 自动生成硬件 ID 列表；玩家运行时不会携带数据库，也不会增加第二个可执行文件：

```powershell
python scripts/generate-driverguard-ini.py --output DriverGuard.generated.ini
python scripts/generate-driverguard-ini.py --output DriverGuard.generated.ini --nvidia-deny-below 551.76
```

第一条命令只会把当前 NVIDIA/AMD/Intel 的 ID 写入注释。只有显式传入 `--*-deny-below` 才会生成覆盖这些 ID 的规则；阈值仍必须结合项目的 Unity/Vulkan 真机测试矩阵验证。`pci.ids` 由 [pciutils](https://github.com/pciutils/pciids) 维护，发布生成数据时必须保留其许可证和声明。

## 平台说明

- Windows 动态加载 `vulkan-1.dll`，Linux 动态加载 `libvulkan.so.1`，因此 Unity Editor 和游戏工程不需要 Vulkan SDK 头文件或链接库。
- Linux 弹窗依次尝试 `zenity` 和 `kdialog`；如果桌面环境没有这些工具，失败原因会输出到 stderr。
- Windows 使用 Task Dialog，并以 `MessageBox` 作为后备；代理静态链接 MSVC Runtime。
- 代理架构必须与 Unity Player 一致，当前提供的产物目标为 `x86_64`。
