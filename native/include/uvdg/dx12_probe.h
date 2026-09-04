#pragma once

#include "uvdg/types.h"

#include <string>

namespace uvdg {

struct D3D12ProbeResult {
    FailureKind failure = FailureKind::None;
    // Highest D3D12 feature level the adapter exposes (11_0, 11_1, 12_0, 12_1).
    std::uint32_t featureLevel = 0;
    GpuInfo gpu;
    std::string reason;
};

// Tries to create a real D3D12 device on the best hardware adapter and reports
// the highest supported feature level. On non-Windows platforms this always
// fails because Direct3D 12 is a Windows-only runtime.
D3D12ProbeResult ProbeD3D12();

// Parses "11_0", "11.0", "12_1", "0xC000", or a decimal value into a
// D3D_FEATURE_LEVEL value. Returns 0 when the text is not recognized.
std::uint32_t ParseFeatureLevel(const std::string& text);

// Formats a D3D_FEATURE_LEVEL value as "11_0", "11_1", "12_0", or "12_1".
std::string FormatFeatureLevel(std::uint32_t featureLevel);

}  // namespace uvdg
