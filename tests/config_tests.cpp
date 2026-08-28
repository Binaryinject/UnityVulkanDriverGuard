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
                  "+DriverDenyList=<999.0|Legacy syntax must be ignored\n"
                  "+DriverDenyList=(DriverVersion=\"<551.76\",RHIName=\"Vulkan\",AdapterNameRegex=\".*GTX 10[0-9]0.*\",DeviceId=\"0x1B80\",DriverId=\"NVIDIA_PROPRIETARY\",SuggestedDriverVersion=\"551.76\",Reason=\"Known Vulkan issue\")\n"
                  "DownloadURL=https://example.invalid/nvidia\n";
    }

    const auto config = uvdg::LoadConfig(path.string());
    std::filesystem::remove(path);
    Require(config.minimumVulkanMajor == 1 && config.minimumVulkanMinor == 1,
            "minimum Vulkan version was not parsed");
    Require(config.driverDenyList.size() == 1, "deny-list rule was not parsed");
    Require(config.driverDenyList[0].deviceIds.size() == 1 &&
            config.driverDenyList[0].driverIds.size() == 1 &&
            config.driverDenyList[0].rhiName == "Vulkan" &&
            config.driverDenyList[0].suggestedVersion == "551.76",
            "deny-list selectors were not parsed");
    Require(uvdg::Matches(uvdg::Version::Parse("546.33"), config.driverDenyList[0]),
            "older driver should match deny-list rule");
    Require(!uvdg::Matches(uvdg::Version::Parse("551.76"), config.driverDenyList[0]),
            "boundary driver should not match a strict less-than rule");

    uvdg::GpuInfo gpu;
    gpu.vendorId = uvdg::kVendorNvidia;
    gpu.deviceId = 0x1B80;
    gpu.driverId = 4;
    gpu.unifiedDriverVersion = uvdg::Version::Parse("546.33");
    gpu.deviceName = "NVIDIA GeForce GTX 1080";
    Require(uvdg::Matches(gpu, config.driverDenyList[0]), "matching GPU should be denied");
    gpu.deviceId = 0x2206;
    Require(!uvdg::Matches(gpu, config.driverDenyList[0]), "different GPU must not be denied");
    gpu.deviceId = 0x1B80;
    gpu.driverId = 3;
    Require(!uvdg::Matches(gpu, config.driverDenyList[0]), "different Vulkan driver must not be denied");
    gpu.driverId = 4;
    gpu.deviceName = "NVIDIA GeForce RTX 4090";
    Require(!uvdg::Matches(gpu, config.driverDenyList[0]), "adapter regex mismatch must not deny");
}
