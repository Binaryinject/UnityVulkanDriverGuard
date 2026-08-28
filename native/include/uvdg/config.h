#pragma once

#include "uvdg/types.h"

#include <string>

namespace uvdg {

Config DefaultConfig();
Config LoadConfig(const std::string& path);
bool Matches(const Version& installed, const DriverRule& rule);
const VendorPolicy& PolicyFor(const Config& config, std::uint32_t vendorId);

}  // namespace uvdg

