#include "uvdg/config.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace uvdg {
namespace {

std::string Trim(std::string value) {
    const auto nonSpace = [](const unsigned char c) { return !std::isspace(c); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), nonSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), nonSpace).base(), value.end());
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        value = value.substr(1, value.size() - 2);
    }
    return value;
}

std::string Lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

DriverRule ParseRule(std::string value) {
    DriverRule result;
    value = Trim(value);
    if (value.rfind("<=", 0) == 0) {
        result.comparison = Comparison::LessOrEqual;
        value.erase(0, 2);
    } else if (value.rfind(">=", 0) == 0) {
        result.comparison = Comparison::GreaterOrEqual;
        value.erase(0, 2);
    } else if (value.rfind("<", 0) == 0) {
        result.comparison = Comparison::Less;
        value.erase(0, 1);
    } else if (value.rfind(">", 0) == 0) {
        result.comparison = Comparison::Greater;
        value.erase(0, 1);
    } else if (value.rfind("=", 0) == 0) {
        result.comparison = Comparison::Equal;
        value.erase(0, 1);
    }

    const auto separator = value.find('|');
    if (separator != std::string::npos) {
        result.reason = Trim(value.substr(separator + 1));
        value.resize(separator);
    }
    result.version = Version::Parse(Trim(value));
    return result;
}

VendorPolicy* FindSection(Config& config, const std::string& section) {
    const auto name = Lower(section);
    if (name == "gpu_nvidia" || name == "nvidia") return &config.nvidia;
    if (name == "gpu_amd" || name == "amd") return &config.amd;
    if (name == "gpu_intel" || name == "intel") return &config.intel;
    if (name == "gpu_other" || name == "other") return &config.other;
    return nullptr;
}

}  // namespace

Config DefaultConfig() {
    Config config;
    config.nvidia.name = "NVIDIA";
    config.nvidia.downloadUrl = "https://www.nvidia.com/Download/index.aspx";
    config.amd.name = "AMD";
    config.amd.downloadUrl = "https://www.amd.com/en/support/download/drivers.html";
    config.intel.name = "Intel";
    config.intel.downloadUrl = "https://www.intel.com/content/www/us/en/download-center/home.html";
    config.other.name = "GPU vendor";
    return config;
}

Config LoadConfig(const std::string& path) {
    Config config = DefaultConfig();
    std::ifstream input(path);
    if (!input) return config;

    VendorPolicy* policy = nullptr;
    std::string line;
    while (std::getline(input, line)) {
        line = Trim(line);
        if (line.empty() || line.front() == ';' || line.front() == '#') continue;
        if (line.front() == '[' && line.back() == ']') {
            const auto section = Trim(line.substr(1, line.size() - 2));
            policy = FindSection(config, section);
            continue;
        }

        const auto equals = line.find('=');
        if (equals == std::string::npos) continue;
        const auto key = Lower(Trim(line.substr(0, equals)));
        const auto value = Trim(line.substr(equals + 1));

        if (!policy) {
            if (key == "minimumvulkanversion") {
                const auto version = Version::Parse(value);
                if (!version.components.empty()) config.minimumVulkanMajor = version.components[0];
                if (version.components.size() > 1) config.minimumVulkanMinor = version.components[1];
            }
            continue;
        }

        if (key == "suggesteddriverversion") {
            policy->suggestedVersionText = value;
            policy->suggestedVersion = Version::Parse(value);
        } else if (key == "downloadurl") {
            policy->downloadUrl = value;
        } else if (key == "driverdenylist" || key == "+driverdenylist") {
            auto rule = ParseRule(value);
            if (!rule.version.components.empty()) policy->denyList.push_back(std::move(rule));
        }
    }
    return config;
}

bool Matches(const Version& installed, const DriverRule& rule) {
    const int comparison = Compare(installed, rule.version);
    switch (rule.comparison) {
        case Comparison::Less: return comparison < 0;
        case Comparison::LessOrEqual: return comparison <= 0;
        case Comparison::Equal: return comparison == 0;
        case Comparison::GreaterOrEqual: return comparison >= 0;
        case Comparison::Greater: return comparison > 0;
    }
    return false;
}

const VendorPolicy& PolicyFor(const Config& config, const std::uint32_t vendorId) {
    if (vendorId == kVendorNvidia) return config.nvidia;
    if (vendorId == kVendorAmd) return config.amd;
    if (vendorId == kVendorIntel) return config.intel;
    return config.other;
}

}  // namespace uvdg

