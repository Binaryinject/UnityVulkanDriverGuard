#include "tests.h"

#include "uvdg/config.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace {

void Require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

}  // namespace

void RunConfigTests() {
    const auto path = std::filesystem::temp_directory_path() / "uvdg-config-test.ini";
    {
        std::ofstream output(path);
        output << "[Global]\nMinimumVulkanVersion=1.1\n"
                  "[GPU_NVIDIA]\nSuggestedDriverVersion=551.76\n"
                  "+DriverDenyList=<551.76|Known Vulkan issue\n"
                  "DownloadURL=https://example.invalid/nvidia\n";
    }

    const auto config = uvdg::LoadConfig(path.string());
    std::filesystem::remove(path);
    Require(config.minimumVulkanMajor == 1 && config.minimumVulkanMinor == 1,
            "minimum Vulkan version was not parsed");
    Require(config.nvidia.denyList.size() == 1, "deny-list rule was not parsed");
    Require(uvdg::Matches(uvdg::Version::Parse("546.33"), config.nvidia.denyList[0]),
            "older driver should match deny-list rule");
    Require(!uvdg::Matches(uvdg::Version::Parse("551.76"), config.nvidia.denyList[0]),
            "boundary driver should not match a strict less-than rule");
}

