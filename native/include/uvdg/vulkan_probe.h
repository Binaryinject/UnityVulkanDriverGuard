#pragma once

#include "uvdg/types.h"

#include <string>

namespace uvdg {

struct ProbeResult {
    FailureKind failure = FailureKind::None;
    GpuInfo gpu;
    std::string reason;
};

ProbeResult ProbeVulkan();
Version DecodeDriverVersion(std::uint32_t vendorId, std::uint32_t rawVersion);
void ApplyPlatformDriverVersion(GpuInfo& gpu);
std::string FormatVulkanVersion(std::uint32_t version);

}  // namespace uvdg
