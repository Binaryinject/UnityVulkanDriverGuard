#include "uvdg/localization.h"

#include "uvdg/dx12_probe.h"
#include "uvdg/vulkan_probe.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace uvdg {
namespace {

constexpr LocalizedText kEnglish{
    "Warning: graphics driver problem",
    "Graphics driver update required",
    "Required Vulkan API is unavailable",
    "Direct3D 12 is unavailable",
    "Update driver", "Continue anyway", "Exit", "GPU", "Installed driver", "Current Vulkan API",
    "Required Vulkan API", "Current D3D12 feature level", "Required D3D12 feature level",
    "or newer", "Recommended driver"};
constexpr LocalizedText kChinese{
    "警告：显卡驱动问题",
    "需要更新显卡驱动",
    "不满足 Vulkan API 要求",
    "不满足 Direct3D 12 要求",
    "更新驱动", "仍然继续", "退出", "显卡", "已安装驱动", "当前 Vulkan API",
    "要求的 Vulkan API", "当前 D3D12 功能级别", "要求的 D3D12 功能级别",
    "或更高版本", "建议驱动"};
constexpr LocalizedText kJapanese{
    "警告：グラフィックスドライバーの問題",
    "グラフィックスドライバーの更新が必要です",
    "必要な Vulkan API を利用できません",
    "Direct3D 12 を利用できません",
    "ドライバーを更新", "このまま続行", "終了", "GPU", "インストール済みドライバー", "現在の Vulkan API",
    "必要な Vulkan API", "現在の D3D12 機能レベル", "必要な D3D12 機能レベル",
    "以降", "推奨ドライバー"};
constexpr LocalizedText kKorean{
    "경고: 그래픽 드라이버 문제",
    "그래픽 드라이버 업데이트 필요",
    "필수 Vulkan API를 사용할 수 없음",
    "Direct3D 12를 사용할 수 없음",
    "드라이버 업데이트", "계속 실행", "종료", "GPU", "설치된 드라이버", "현재 Vulkan API",
    "필수 Vulkan API", "현재 D3D12 기능 수준", "필수 D3D12 기능 수준",
    "이상", "권장 드라이버"};

std::string VulkanUnsupported(const Language language, const PreflightResult& result) {
    const std::string version = FormatVulkanVersion(result.gpu.apiVersion);
    const std::string required = std::to_string(result.requiredVulkanMajor) + "." +
                                 std::to_string(result.requiredVulkanMinor);
    switch (language) {
        case Language::Chinese:
            return "此显卡仅支持 Vulkan " + version + "，游戏需要 Vulkan " + required + " 或更高版本。";
        case Language::Japanese:
            return "この GPU が対応する Vulkan は " + version + " です。このゲームには Vulkan " + required + " 以降が必要です。";
        case Language::Korean:
            return "이 GPU는 Vulkan " + version + "을(를) 지원합니다. 게임에는 Vulkan " + required + " 이상이 필요합니다.";
        default:
            return "This GPU exposes Vulkan " + version + ", but this game requires Vulkan " + required + " or newer.";
    }
}

std::string D3D12FeatureLevelUnsupported(const Language language, const PreflightResult& result) {
    const std::string detected = FormatFeatureLevel(result.gpu.d3d12FeatureLevel);
    const std::string required = FormatFeatureLevel(result.requiredFeatureLevel);
    switch (language) {
        case Language::Chinese:
            return "此显卡最高支持 Direct3D 12 功能级别 " + detected + "，游戏需要 " + required + " 或更高版本。";
        case Language::Japanese:
            return "この GPU が対応する Direct3D 12 機能レベルは " + detected + " です。このゲームには " + required + " 以降が必要です。";
        case Language::Korean:
            return "이 GPU는 Direct3D 12 기능 수준 " + detected + "을(를) 지원합니다. 게임에는 " + required + " 이상이 필요합니다.";
        default:
            return "This GPU supports Direct3D 12 feature level " + detected + ", but this game requires " + required + " or newer.";
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
        return VulkanUnsupported(language, result);
    }
    if (result.failure == FailureKind::D3D12FeatureLevelUnsupported) {
        return D3D12FeatureLevelUnsupported(language, result);
    }

    switch (language) {
        case Language::Chinese:
            if (result.failure == FailureKind::DriverDenied) return "已安装的显卡驱动存在已知的 Vulkan 兼容性问题，请更新驱动后再启动游戏。";
            if (result.failure == FailureKind::VulkanLoaderMissing) return "未找到 Vulkan Loader，请安装或更新显卡驱动。";
            if (result.failure == FailureKind::NoPhysicalDevice) return "未找到支持 Vulkan 的显卡。";
            if (result.failure == FailureKind::D3D12Unavailable) return "未找到支持 Direct3D 12 的显卡或运行时，请更新显卡驱动。";
            return "Vulkan 初始化失败，请安装或更新显卡驱动。";
        case Language::Japanese:
            if (result.failure == FailureKind::DriverDenied) return "インストール済みのグラフィックスドライバーには既知の Vulkan 互換性問題があります。ドライバーを更新してください。";
            if (result.failure == FailureKind::VulkanLoaderMissing) return "Vulkan Loader が見つかりません。グラフィックスドライバーをインストールまたは更新してください。";
            if (result.failure == FailureKind::NoPhysicalDevice) return "Vulkan 対応 GPU が見つかりません。";
            if (result.failure == FailureKind::D3D12Unavailable) return "Direct3D 12 対応 GPU が見つかりません。グラフィックスドライバーを更新してください。";
            return "Vulkan の初期化に失敗しました。グラフィックスドライバーをインストールまたは更新してください。";
        case Language::Korean:
            if (result.failure == FailureKind::DriverDenied) return "설치된 그래픽 드라이버에 알려진 Vulkan 호환성 문제가 있습니다. 드라이버를 업데이트해 주세요.";
            if (result.failure == FailureKind::VulkanLoaderMissing) return "Vulkan Loader를 찾을 수 없습니다. 그래픽 드라이버를 설치하거나 업데이트해 주세요.";
            if (result.failure == FailureKind::NoPhysicalDevice) return "Vulkan을 지원하는 GPU를 찾을 수 없습니다.";
            if (result.failure == FailureKind::D3D12Unavailable) return "Direct3D 12를 지원하는 GPU를 찾을 수 없습니다. 그래픽 드라이버를 업데이트해 주세요.";
            return "Vulkan 초기화에 실패했습니다. 그래픽 드라이버를 설치하거나 업데이트해 주세요.";
        default:
            return result.reason;
    }
}

}  // namespace uvdg
