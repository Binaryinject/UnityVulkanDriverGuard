#include "uvdg/preflight.h"

#include "uvdg/config.h"
#include "uvdg/vulkan_probe.h"

#include <sstream>

namespace uvdg {

PreflightResult RunPreflight(const std::string& configPath) {
    const Config config = LoadConfig(configPath);
    const ProbeResult probe = ProbeVulkan();

    PreflightResult result;
    result.failure = probe.failure;
    result.gpu = probe.gpu;
    result.reason = probe.reason;
    if (probe.failure != FailureKind::None) return result;

    const std::uint32_t major = (probe.gpu.apiVersion >> 22) & 0x7f;
    const std::uint32_t minor = (probe.gpu.apiVersion >> 12) & 0x3ff;
    if (major < config.minimumVulkanMajor ||
        (major == config.minimumVulkanMajor && minor < config.minimumVulkanMinor)) {
        result.failure = FailureKind::VulkanVersionUnsupported;
        std::ostringstream reason;
        reason << "This GPU exposes Vulkan " << FormatVulkanVersion(probe.gpu.apiVersion)
               << ", but this game requires Vulkan " << config.minimumVulkanMajor
               << '.' << config.minimumVulkanMinor << " or newer.";
        result.reason = reason.str();
    }

    const VendorPolicy& policy = PolicyFor(config, probe.gpu.vendorId);
    result.suggestedVersion = policy.suggestedVersionText;
    result.downloadUrl = policy.downloadUrl;
    if (result.failure != FailureKind::None) return result;

    for (const DriverRule& rule : config.driverDenyList) {
        if (!Matches(probe.gpu, rule)) continue;
        result.failure = FailureKind::DriverDenied;
        result.reason = rule.reason.empty()
            ? "The installed graphics driver is on this game's deny list."
            : rule.reason;
        if (!rule.suggestedVersion.empty()) result.suggestedVersion = rule.suggestedVersion;
        if (!rule.downloadUrl.empty()) result.downloadUrl = rule.downloadUrl;
        break;
    }
    return result;
}

}  // namespace uvdg
