#include "uvdg/vulkan_probe.h"

#include <array>
#include <cstddef>
#include <cstring>
#include <vector>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace uvdg {
namespace {

using VkResult = std::int32_t;
using VkInstance = struct VkInstance_T*;
using VkPhysicalDevice = struct VkPhysicalDevice_T*;
using PFN_vkVoidFunction = void (*)();

constexpr VkResult VK_SUCCESS = 0;
constexpr std::uint32_t VK_STRUCTURE_TYPE_APPLICATION_INFO = 0;
constexpr std::uint32_t VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO = 1;
constexpr std::uint32_t VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU = 2;
constexpr std::uint32_t VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU = 1;

struct VkApplicationInfo {
    std::uint32_t sType;
    const void* pNext;
    const char* pApplicationName;
    std::uint32_t applicationVersion;
    const char* pEngineName;
    std::uint32_t engineVersion;
    std::uint32_t apiVersion;
};

struct VkInstanceCreateInfo {
    std::uint32_t sType;
    const void* pNext;
    std::uint32_t flags;
    const VkApplicationInfo* pApplicationInfo;
    std::uint32_t enabledLayerCount;
    const char* const* ppEnabledLayerNames;
    std::uint32_t enabledExtensionCount;
    const char* const* ppEnabledExtensionNames;
};

using PFN_vkGetInstanceProcAddr = PFN_vkVoidFunction (*)(VkInstance, const char*);
using PFN_vkCreateInstance = VkResult (*)(const VkInstanceCreateInfo*, const void*, VkInstance*);
using PFN_vkDestroyInstance = void (*)(VkInstance, const void*);
using PFN_vkEnumeratePhysicalDevices = VkResult (*)(VkInstance, std::uint32_t*, VkPhysicalDevice*);
using PFN_vkGetPhysicalDeviceProperties = void (*)(VkPhysicalDevice, void*);

class VulkanLibrary {
public:
    VulkanLibrary() {
#if defined(_WIN32)
        handle_ = LoadLibraryW(L"vulkan-1.dll");
#else
        handle_ = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
#endif
    }

    ~VulkanLibrary() {
        if (!handle_) return;
#if defined(_WIN32)
        FreeLibrary(static_cast<HMODULE>(handle_));
#else
        dlclose(handle_);
#endif
    }

    void* Symbol(const char* name) const {
        if (!handle_) return nullptr;
#if defined(_WIN32)
        return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle_), name));
#else
        return dlsym(handle_, name);
#endif
    }

    explicit operator bool() const { return handle_ != nullptr; }

private:
#if defined(_WIN32)
    void* handle_ = nullptr;
#else
    void* handle_ = nullptr;
#endif
};

template <typename T>
T Read(const std::byte* data, const std::size_t offset) {
    T value{};
    std::memcpy(&value, data + offset, sizeof(T));
    return value;
}

int DeviceScore(const std::uint32_t type) {
    if (type == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) return 200;
    if (type == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) return 100;
    return 0;
}

GpuInfo ReadGpuInfo(const PFN_vkGetPhysicalDeviceProperties getProperties,
                    const VkPhysicalDevice device) {
    alignas(16) std::array<std::byte, 4096> storage{};
    getProperties(device, storage.data());

    GpuInfo result;
    result.apiVersion = Read<std::uint32_t>(storage.data(), 0);
    result.rawDriverVersion = Read<std::uint32_t>(storage.data(), 4);
    result.vendorId = Read<std::uint32_t>(storage.data(), 8);
    result.deviceId = Read<std::uint32_t>(storage.data(), 12);
    result.deviceType = Read<std::uint32_t>(storage.data(), 16);
    const auto* name = reinterpret_cast<const char*>(storage.data() + 20);
    result.deviceName.assign(name, strnlen(name, 256));
    result.unifiedDriverVersion = DecodeDriverVersion(result.vendorId, result.rawDriverVersion);
    ApplyPlatformDriverVersion(result);
    return result;
}

}  // namespace

ProbeResult ProbeVulkan() {
    ProbeResult result;
    VulkanLibrary library;
    if (!library) {
        result.failure = FailureKind::VulkanLoaderMissing;
        result.reason = "The Vulkan Loader is not installed.";
        return result;
    }

    const auto getInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
        library.Symbol("vkGetInstanceProcAddr"));
    if (!getInstanceProcAddr) {
        result.failure = FailureKind::VulkanInitializationFailed;
        result.reason = "The Vulkan Loader does not export vkGetInstanceProcAddr.";
        return result;
    }

    const auto createInstance = reinterpret_cast<PFN_vkCreateInstance>(
        getInstanceProcAddr(nullptr, "vkCreateInstance"));
    if (!createInstance) {
        result.failure = FailureKind::VulkanInitializationFailed;
        result.reason = "The Vulkan Loader does not export vkCreateInstance.";
        return result;
    }

    const VkApplicationInfo applicationInfo{
        VK_STRUCTURE_TYPE_APPLICATION_INFO, nullptr, "UnityVulkanDriverGuard", 1,
        "UnityVulkanDriverGuard", 1, (1u << 22)};
    const VkInstanceCreateInfo createInfo{
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, nullptr, 0, &applicationInfo,
        0, nullptr, 0, nullptr};

    VkInstance instance = nullptr;
    const VkResult createResult = createInstance(&createInfo, nullptr, &instance);
    if (createResult != VK_SUCCESS || !instance) {
        result.failure = FailureKind::VulkanInitializationFailed;
        result.reason = "Vulkan could not create a minimal 1.0 instance (VkResult " +
                        std::to_string(createResult) + ").";
        return result;
    }

    const auto destroyInstance = reinterpret_cast<PFN_vkDestroyInstance>(
        getInstanceProcAddr(instance, "vkDestroyInstance"));
    const auto enumerateDevices = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(
        getInstanceProcAddr(instance, "vkEnumeratePhysicalDevices"));
    const auto getProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(
        getInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties"));

    if (!enumerateDevices || !getProperties) {
        if (destroyInstance) destroyInstance(instance, nullptr);
        result.failure = FailureKind::VulkanInitializationFailed;
        result.reason = "Required Vulkan physical-device functions are unavailable.";
        return result;
    }

    std::uint32_t deviceCount = 0;
    VkResult enumerateResult = enumerateDevices(instance, &deviceCount, nullptr);
    if (enumerateResult != VK_SUCCESS || deviceCount == 0) {
        if (destroyInstance) destroyInstance(instance, nullptr);
        result.failure = FailureKind::NoPhysicalDevice;
        result.reason = "No Vulkan-capable physical device was found.";
        return result;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    enumerateResult = enumerateDevices(instance, &deviceCount, devices.data());
    if (enumerateResult != VK_SUCCESS || deviceCount == 0) {
        if (destroyInstance) destroyInstance(instance, nullptr);
        result.failure = FailureKind::NoPhysicalDevice;
        result.reason = "Vulkan physical-device enumeration failed (VkResult " +
                        std::to_string(enumerateResult) + ").";
        return result;
    }

    int bestScore = -1;
    for (std::uint32_t i = 0; i < deviceCount; ++i) {
        auto gpu = ReadGpuInfo(getProperties, devices[i]);
        const int score = DeviceScore(gpu.deviceType);
        if (score > bestScore) {
            bestScore = score;
            result.gpu = std::move(gpu);
        }
    }

    if (destroyInstance) destroyInstance(instance, nullptr);
    return result;
}

}  // namespace uvdg

#if !defined(_WIN32)
namespace uvdg {
void ApplyPlatformDriverVersion(GpuInfo&) {}
}  // namespace uvdg
#endif

