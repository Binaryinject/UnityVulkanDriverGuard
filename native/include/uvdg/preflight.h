#pragma once

#include "uvdg/types.h"

#include <string>

namespace uvdg {

PreflightResult RunPreflight(const std::string& configPath);
// Returns true only when the user accepts an eligible driver warning.
bool ShowFailureDialog(const PreflightResult& result);
std::string ExecutableDirectory();

}  // namespace uvdg
