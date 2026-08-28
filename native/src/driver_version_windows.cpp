#include "uvdg/vulkan_probe.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <algorithm>
#include <cwctype>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace uvdg {
namespace {

std::wstring HexId(const wchar_t* prefix, const std::uint32_t value) {
    std::wostringstream stream;
    stream << prefix << std::uppercase << std::hex << std::setw(4) << std::setfill(L'0') << value;
    return stream.str();
}

std::wstring Lower(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](const wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
    return value;
}

std::string Utf8(const std::wstring& value) {
    if (value.empty()) return {};
    const int count = WideCharToMultiByte(CP_UTF8, 0, value.data(),
                                          static_cast<int>(value.size()),
                                          nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<std::size_t>(count), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                        result.data(), count, nullptr, nullptr);
    return result;
}

bool QueryString(HKEY key, const wchar_t* name, std::wstring& value) {
    DWORD bytes = 0;
    if (RegGetValueW(key, nullptr, name, RRF_RT_REG_SZ, nullptr, nullptr, &bytes) != ERROR_SUCCESS ||
        bytes < sizeof(wchar_t)) {
        return false;
    }
    std::vector<wchar_t> buffer(bytes / sizeof(wchar_t));
    if (RegGetValueW(key, nullptr, name, RRF_RT_REG_SZ, nullptr, buffer.data(), &bytes) != ERROR_SUCCESS) {
        return false;
    }
    value.assign(buffer.data());
    return !value.empty();
}

bool OpenAdapterKey(const GpuInfo& gpu, HKEY& key) {
    const std::wstring vendor = Lower(HexId(L"ven_", gpu.vendorId));
    const std::wstring deviceId = Lower(HexId(L"dev_", gpu.deviceId));
    for (DWORD index = 0;; ++index) {
        DISPLAY_DEVICEW device{};
        device.cb = sizeof(device);
        if (!EnumDisplayDevicesW(nullptr, index, &device, 0)) break;
        const std::wstring id = Lower(device.DeviceID);
        if (id.find(vendor) == std::wstring::npos || id.find(deviceId) == std::wstring::npos) continue;

        std::wstring path = device.DeviceKey;
        const std::wstring prefix = L"\\Registry\\Machine\\";
        if (Lower(path).rfind(Lower(prefix), 0) == 0) path.erase(0, prefix.size());
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, path.c_str(), 0, KEY_READ, &key) == ERROR_SUCCESS) {
            return true;
        }
    }
    return false;
}

Version NvidiaUnified(const Version& internal) {
    if (internal.components.size() < 4) return {};
    const auto third = internal.components[internal.components.size() - 2];
    const auto fourth = internal.components.back();
    return Version{{(third % 10) * 100 + fourth / 100, fourth % 100}};
}

Version IntelUnified(const Version& internal) {
    if (internal.components.size() < 2) return {};
    return Version{{internal.components[internal.components.size() - 2], internal.components.back()}};
}

}  // namespace

void ApplyPlatformDriverVersion(GpuInfo& gpu) {
    HKEY key = nullptr;
    if (!OpenAdapterKey(gpu, key)) return;

    std::wstring value;
    if (gpu.vendorId == kVendorAmd &&
        (QueryString(key, L"RadeonSoftwareVersion", value) ||
         QueryString(key, L"Catalyst_Version", value))) {
        gpu.unifiedDriverVersion = Version::Parse(Utf8(value));
        gpu.driverInfo = Utf8(value);
        RegCloseKey(key);
        return;
    }

    if (QueryString(key, L"DriverVersion", value)) {
        const Version internal = Version::Parse(Utf8(value));
        Version unified;
        if (gpu.vendorId == kVendorNvidia) unified = NvidiaUnified(internal);
        if (gpu.vendorId == kVendorIntel) unified = IntelUnified(internal);
        if (!unified.components.empty()) gpu.unifiedDriverVersion = std::move(unified);
        gpu.driverInfo = Utf8(value);
    }
    RegCloseKey(key);
}

}  // namespace uvdg
