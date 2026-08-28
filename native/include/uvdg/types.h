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
    Comparison comparison = Comparison::Less;
    Version version;
    std::string reason;
};

struct VendorPolicy {
    std::string name;
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
};

struct GpuInfo {
    std::uint32_t apiVersion = 0;
    std::uint32_t rawDriverVersion = 0;
    std::uint32_t vendorId = 0;
    std::uint32_t deviceId = 0;
    std::uint32_t deviceType = 0;
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
    std::string reason;
    std::string suggestedVersion;
    std::string downloadUrl;

    bool Passed() const { return failure == FailureKind::None; }
};

}  // namespace uvdg

