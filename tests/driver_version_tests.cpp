#include "tests.h"

#include "uvdg/localization.h"
#include "uvdg/vulkan_probe.h"

#include <stdexcept>

namespace {

void Require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

}  // namespace

void RunDriverVersionTests() {
    const std::uint32_t nvidiaRaw = (516u << 22) | (25u << 14) | (3u << 6) | 7u;
    const auto nvidia = uvdg::DecodeDriverVersion(uvdg::kVendorNvidia, nvidiaRaw);
    Require(nvidia.ToString() == "516.25.3.7", "NVIDIA Vulkan encoding is wrong");

    const std::uint32_t standardRaw = (24u << 22) | (3u << 12) | 1u;
    const auto standard = uvdg::DecodeDriverVersion(uvdg::kVendorAmd, standardRaw);
    Require(standard.ToString() == "24.3.1", "standard Vulkan encoding is wrong");

    Require(uvdg::Compare(uvdg::Version::Parse("1.2"), uvdg::Version::Parse("1.2.0")) == 0,
            "missing version components should compare as zero");
    Require(uvdg::FormatVulkanVersion((1u << 22) | (1u << 12)) == "1.1.0",
            "Vulkan API version formatting is wrong");

    Require(uvdg::LanguageFromLocale("zh-CN") == uvdg::Language::Chinese,
            "Chinese locale detection is wrong");
    Require(uvdg::LanguageFromLocale("ja_JP.UTF-8") == uvdg::Language::Japanese,
            "Japanese locale detection is wrong");
    Require(uvdg::LanguageFromLocale("ko-KR") == uvdg::Language::Korean,
            "Korean locale detection is wrong");
    Require(uvdg::LanguageFromLocale("fr-FR") == uvdg::Language::English,
            "non-CJK locales must fall back to English");
}
