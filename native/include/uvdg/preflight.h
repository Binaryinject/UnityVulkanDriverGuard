#pragma once

#include "uvdg/types.h"

#include <string>

namespace uvdg {

PreflightResult RunPreflight(const std::string& configPath);
bool ShowFailureDialog(const PreflightResult& result);
std::string ExecutableDirectory();

}  // namespace uvdg

