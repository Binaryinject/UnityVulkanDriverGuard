#include "uvdg/preflight.h"
#include "uvdg/localization.h"
#include "uvdg/vulkan_probe.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>

#include <sstream>
#include <string>
#include <iterator>
#include <vector>

namespace uvdg {
namespace {

std::wstring Wide(const std::string& value) {
    if (value.empty()) return {};
    const int count = MultiByteToWideChar(CP_UTF8, 0, value.data(),
                                          static_cast<int>(value.size()), nullptr, 0);
    std::wstring result(static_cast<std::size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                        result.data(), count);
    return result;
}

Language SystemLanguage() {
    wchar_t localeName[LOCALE_NAME_MAX_LENGTH]{};
    if (!GetUserDefaultLocaleName(localeName, LOCALE_NAME_MAX_LENGTH)) {
        return Language::English;
    }
    const int length = static_cast<int>(wcslen(localeName));
    const int bytes = WideCharToMultiByte(CP_UTF8, 0, localeName, length, nullptr, 0,
                                          nullptr, nullptr);
    std::string locale(static_cast<std::size_t>(bytes), '\0');
    if (bytes > 0) {
        WideCharToMultiByte(CP_UTF8, 0, localeName, length, locale.data(), bytes,
                            nullptr, nullptr);
    }
    return LanguageFromLocale(locale);
}

}  // namespace

std::string ExecutableDirectory() {
    std::vector<wchar_t> buffer(1024);
    for (;;) {
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(),
                                                static_cast<DWORD>(buffer.size()));
        if (length == 0) return ".";
        if (length < buffer.size() - 1) {
            std::wstring path(buffer.data(), length);
            const auto slash = path.find_last_of(L"\\/");
            if (slash != std::wstring::npos) path.resize(slash);
            const int bytes = WideCharToMultiByte(CP_UTF8, 0, path.data(),
                                                  static_cast<int>(path.size()),
                                                  nullptr, 0, nullptr, nullptr);
            std::string utf8(static_cast<std::size_t>(bytes), '\0');
            WideCharToMultiByte(CP_UTF8, 0, path.data(), static_cast<int>(path.size()),
                                utf8.data(), bytes, nullptr, nullptr);
            return utf8;
        }
        buffer.resize(buffer.size() * 2);
    }
}

bool ShowFailureDialog(const PreflightResult& result) {
    const Language language = SystemLanguage();
    const LocalizedText& text = Text(language);
    std::ostringstream details;
    details << LocalizedReason(language, result);
    if (!result.gpu.deviceName.empty()) {
        details << "\n\n" << text.gpu << ": " << result.gpu.deviceName;
        details << "\n" << text.installedDriver << ": "
                << result.gpu.unifiedDriverVersion.ToString();
        details << "\n" << text.vulkanApi << ": " << FormatVulkanVersion(result.gpu.apiVersion);
    }
    if (!result.suggestedVersion.empty()) {
        details << "\n" << text.recommendedDriver << ": " << result.suggestedVersion;
    }

    const std::wstring windowTitle = Wide(text.windowTitle);
    const std::wstring title = Wide(result.failure == FailureKind::DriverDenied
                                        ? text.driverTitle : text.vulkanTitle);
    const std::wstring content = Wide(details.str());
    const std::wstring updateDriver = Wide(text.updateDriver);
    const std::wstring exit = Wide(text.exit);
    const TASKDIALOG_BUTTON buttons[] = {
        {1001, updateDriver.c_str()}, {1002, exit.c_str()}};

    TASKDIALOGCONFIG config{};
    config.cbSize = sizeof(config);
    config.dwFlags = TDF_SIZE_TO_CONTENT | TDF_ALLOW_DIALOG_CANCELLATION;
    config.pszWindowTitle = windowTitle.c_str();
    config.pszMainIcon = TD_WARNING_ICON;
    config.pszMainInstruction = title.c_str();
    config.pszContent = content.c_str();
    config.cButtons = static_cast<UINT>(std::size(buttons));
    config.pButtons = buttons;
    config.nDefaultButton = 1001;

    int selected = 1002;
    const HRESULT dialogResult = TaskDialogIndirect(&config, &selected, nullptr, nullptr);
    if (FAILED(dialogResult)) {
        selected = MessageBoxW(nullptr, content.c_str(), title.c_str(),
                               MB_ICONWARNING | MB_YESNO | MB_SETFOREGROUND) == IDYES
            ? 1001 : 1002;
    }

    if (selected == 1001 && !result.downloadUrl.empty()) {
        const auto url = Wide(result.downloadUrl);
        ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        return true;
    }
    return false;
}

}  // namespace uvdg
