#include "uvdg/vulkan_probe.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace uvdg {

Version Version::Parse(const std::string& text) {
    Version result;
    std::uint64_t value = 0;
    bool reading = false;
    for (const unsigned char character : text) {
        if (std::isdigit(character)) {
            value = std::min<std::uint64_t>(value * 10 + (character - '0'), UINT32_MAX);
            reading = true;
        } else if (reading) {
            result.components.push_back(static_cast<std::uint32_t>(value));
            value = 0;
            reading = false;
        }
    }
    if (reading) {
        result.components.push_back(static_cast<std::uint32_t>(value));
    }
    return result;
}

std::string Version::ToString() const {
    std::ostringstream stream;
    for (std::size_t i = 0; i < components.size(); ++i) {
        if (i != 0) {
            stream << '.';
        }
        stream << components[i];
    }
    return stream.str();
}

int Compare(const Version& left, const Version& right) {
    const std::size_t count = std::max(left.components.size(), right.components.size());
    for (std::size_t i = 0; i < count; ++i) {
        const auto a = i < left.components.size() ? left.components[i] : 0;
        const auto b = i < right.components.size() ? right.components[i] : 0;
        if (a < b) return -1;
        if (a > b) return 1;
    }
    return 0;
}

Version DecodeDriverVersion(const std::uint32_t vendorId, const std::uint32_t rawVersion) {
    Version result;
    if (vendorId == kVendorNvidia) {
        result.components = {
            rawVersion >> 22,
            (rawVersion >> 14) & 0xff,
            (rawVersion >> 6) & 0xff,
            rawVersion & 0x3f};
        return result;
    }

#if defined(_WIN32)
    if (vendorId == kVendorIntel) {
        result.components = {rawVersion >> 14, rawVersion & 0x3fff};
        return result;
    }
#endif

    result.components = {
        rawVersion >> 22,
        (rawVersion >> 12) & 0x3ff,
        rawVersion & 0xfff};
    return result;
}

std::string FormatVulkanVersion(const std::uint32_t version) {
    std::ostringstream stream;
    stream << ((version >> 22) & 0x7f) << '.' << ((version >> 12) & 0x3ff) << '.'
           << (version & 0xfff);
    return stream.str();
}

}  // namespace uvdg
