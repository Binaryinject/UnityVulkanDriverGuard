#include "uvdg/localization.h"

#include "uvdg/vulkan_probe.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace uvdg {
namespace {

constexpr LocalizedText kEnglish{
    "Warning: graphics driver problem",
    "Graphics driver update required",
    "Vulkan 1.1 is required",
    "Update driver", "Exit", "GPU", "Installed driver", "Vulkan API", "Recommended driver"};
constexpr LocalizedText kChinese{
    "警告：显卡驱动问题",
    "需要更新显卡驱动",
    "需要 Vulkan 1.1",
    "更新驱动", "退出", "显卡", "已安装驱动", "Vulkan API", "建议驱动"};
constexpr LocalizedText kJapanese{
    "警告：グラフィックスドライバーの問題",
    "グラフィックスドライバーの更新が必要です",
    "Vulkan 1.1 が必要です",
    "ドライバーを更新", "終了", "GPU", "インストール済みドライバー", "Vulkan API", "推奨ドライバー"};
constexpr LocalizedText kKorean{
    "경고: 그래픽 드라이버 문제",
    "그래픽 드라이버 업데이트 필요",
    "Vulkan 1.1 필요",
    "드라이버 업데이트", "종료", "GPU", "설치된 드라이버", "Vulkan API", "권장 드라이버"};

std::string VulkanUnsupported(const Language language, const GpuInfo& gpu) {
    const std::string version = FormatVulkanVersion(gpu.apiVersion);
    switch (language) {
        case Language::Chinese:
            return "此显卡仅支持 Vulkan " + version + "，游戏需要 Vulkan 1.1 或更高版本。";
        case Language::Japanese:
            return "この GPU が対応する Vulkan は " + version + " です。このゲームには Vulkan 1.1 以降が必要です。";
        case Language::Korean:
            return "이 GPU는 Vulkan " + version + "을(를) 지원합니다. 게임에는 Vulkan 1.1 이상이 필요합니다.";
        default:
            return "This GPU exposes Vulkan " + version + ", but this game requires Vulkan 1.1 or newer.";
    }
}

}  // namespace

Language LanguageFromLocale(const std::string& localeName) {
    std::string locale = localeName;
    std::transform(locale.begin(), locale.end(), locale.begin(),
                   [](const unsigned char value) { return static_cast<char>(std::tolower(value)); });
    if (locale.rfind("zh", 0) == 0) return Language::Chinese;
    if (locale.rfind("ja", 0) == 0) return Language::Japanese;
    if (locale.rfind("ko", 0) == 0) return Language::Korean;
    return Language::English;
}

const LocalizedText& Text(const Language language) {
    switch (language) {
        case Language::Chinese: return kChinese;
        case Language::Japanese: return kJapanese;
        case Language::Korean: return kKorean;
        default: return kEnglish;
    }
}

std::string LocalizedReason(const Language language, const PreflightResult& result) {
    if (language == Language::English && !result.reason.empty()) return result.reason;
    if (result.failure == FailureKind::VulkanVersionUnsupported) {
        return VulkanUnsupported(language, result.gpu);
    }

    switch (language) {
        case Language::Chinese:
            if (result.failure == FailureKind::DriverDenied) return "已安装的显卡驱动存在已知的 Vulkan 兼容性问题，请更新驱动后再启动游戏。";
            if (result.failure == FailureKind::VulkanLoaderMissing) return "未找到 Vulkan Loader，请安装或更新显卡驱动。";
            if (result.failure == FailureKind::NoPhysicalDevice) return "未找到支持 Vulkan 的显卡。";
            return "Vulkan 初始化失败，请安装或更新显卡驱动。";
        case Language::Japanese:
            if (result.failure == FailureKind::DriverDenied) return "インストール済みのグラフィックスドライバーには既知の Vulkan 互換性問題があります。ドライバーを更新してください。";
            if (result.failure == FailureKind::VulkanLoaderMissing) return "Vulkan Loader が見つかりません。グラフィックスドライバーをインストールまたは更新してください。";
            if (result.failure == FailureKind::NoPhysicalDevice) return "Vulkan 対応 GPU が見つかりません。";
            return "Vulkan の初期化に失敗しました。グラフィックスドライバーをインストールまたは更新してください。";
        case Language::Korean:
            if (result.failure == FailureKind::DriverDenied) return "설치된 그래픽 드라이버에 알려진 Vulkan 호환성 문제가 있습니다. 드라이버를 업데이트해 주세요.";
            if (result.failure == FailureKind::VulkanLoaderMissing) return "Vulkan Loader를 찾을 수 없습니다. 그래픽 드라이버를 설치하거나 업데이트해 주세요.";
            if (result.failure == FailureKind::NoPhysicalDevice) return "Vulkan을 지원하는 GPU를 찾을 수 없습니다.";
            return "Vulkan 초기화에 실패했습니다. 그래픽 드라이버를 설치하거나 업데이트해 주세요.";
        default:
            return result.reason;
    }
}

}  // namespace uvdg
