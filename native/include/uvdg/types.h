#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace uvdg {

constexpr std::uint32_t kVendorNvidia = 0x10de;
constexpr std::uint32_t kVendorAmd = 0x1002;
constexpr std::uint32_t kVendorIntel = 0x8086;

struct Version {
    std::vector<std::uint32_t> components;

    static Version Parse(const std::string& text);
    std::string ToString() const;
};

int Compare(const Version& left, const Version& right);

enum class Comparison {
    Less,
    LessOrEqual,
    Equal,
    GreaterOrEqual,
    Greater
};

struct DriverRule {
    struct IdRange {
        std::uint32_t first = 0;
        std::uint32_t last = 0;
    };

    std::uint32_t vendorId = 0;
    std::string rhiName;
    std::string adapterNameRegex;
    bool allDeviceIds = false;
    bool allDriverIds = false;
    std::vector<IdRange> deviceIds;
    std::vector<std::uint32_t> driverIds;
    Comparison comparison = Comparison::Less;
    Version version;
    std::string reason;
    std::string suggestedVersion;
    std::string downloadUrl;
};

struct VendorPolicy {
    std::string name;
    Version minimumVersion;
    std::string minimumVersionText;
    Version suggestedVersion;
    std::string suggestedVersionText;
    std::string downloadUrl;
    std::vector<DriverRule> denyList;
};

struct Config {
    std::uint32_t minimumVulkanMajor = 1;
    std::uint32_t minimumVulkanMinor = 1;
    VendorPolicy nvidia;
    VendorPolicy amd;
    VendorPolicy intel;
    VendorPolicy other;
    std::vector<DriverRule> driverDenyList;
};

struct GpuInfo {
    std::uint32_t apiVersion = 0;
    std::uint32_t rawDriverVersion = 0;
    std::uint32_t vendorId = 0;
    std::uint32_t deviceId = 0;
    std::uint32_t deviceType = 0;
    std::uint32_t driverId = 0;
    std::string deviceName;
    std::string driverName;
    std::string driverInfo;
    Version unifiedDriverVersion;
};

enum class FailureKind {
    None,
    VulkanLoaderMissing,
    VulkanInitializationFailed,
    NoPhysicalDevice,
    VulkanVersionUnsupported,
    DriverDenied
};

struct PreflightResult {
    FailureKind failure = FailureKind::None;
    GpuInfo gpu;
    std::uint32_t requiredVulkanMajor = 1;
    std::uint32_t requiredVulkanMinor = 1;
    std::string reason;
    std::string suggestedVersion;
    std::string downloadUrl;

    bool Passed() const { return failure == FailureKind::None; }
    bool CanContinue() const {
        const std::uint32_t major = (gpu.apiVersion >> 22) & 0x7f;
        const std::uint32_t minor = (gpu.apiVersion >> 12) & 0x3ff;
        return failure == FailureKind::DriverDenied &&
               (major > 1 || (major == 1 && minor >= 1));
    }
};

}  // namespace uvdg
