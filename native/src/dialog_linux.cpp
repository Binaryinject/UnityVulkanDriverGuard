#include "uvdg/preflight.h"

#include "uvdg/localization.h"
#include "uvdg/vulkan_probe.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace uvdg {
namespace {

int Run(const std::vector<std::string>& arguments) {
    if (arguments.empty()) return -1;
    const pid_t child = fork();
    if (child < 0) return -1;
    if (child == 0) {
        std::vector<char*> argv;
        argv.reserve(arguments.size() + 1);
        for (const auto& value : arguments) argv.push_back(const_cast<char*>(value.c_str()));
        argv.push_back(nullptr);
        execvp(argv[0], argv.data());
        _exit(127);
    }
    int status = 0;
    if (waitpid(child, &status, 0) < 0 || !WIFEXITED(status)) return -1;
    return WEXITSTATUS(status);
}

Language SystemLanguage() {
    const char* locale = std::getenv("LC_ALL");
    if (!locale || !*locale) locale = std::getenv("LC_MESSAGES");
    if (!locale || !*locale) locale = std::getenv("LANG");
    return LanguageFromLocale(locale ? locale : "en");
}

std::string Details(const PreflightResult& result, const Language language) {
    const LocalizedText& text = Text(language);
    std::ostringstream stream;
    stream << LocalizedReason(language, result);
    if (!result.gpu.deviceName.empty()) {
        stream << "\n\n" << text.gpu << ": " << result.gpu.deviceName
               << "\n" << text.installedDriver << ": "
               << result.gpu.unifiedDriverVersion.ToString()
               << "\n" << text.vulkanApi << ": " << FormatVulkanVersion(result.gpu.apiVersion);
    }
    if (!result.suggestedVersion.empty()) {
        stream << "\n" << text.recommendedDriver << ": " << result.suggestedVersion;
    }
    return stream.str();
}

}  // namespace

std::string ExecutableDirectory() {
    std::array<char, 4096> path{};
    const ssize_t length = readlink("/proc/self/exe", path.data(), path.size() - 1);
    if (length <= 0) return ".";
    std::string result(path.data(), static_cast<std::size_t>(length));
    const auto slash = result.find_last_of('/');
    return slash == std::string::npos ? "." : result.substr(0, slash);
}

bool ShowFailureDialog(const PreflightResult& result) {
    const Language language = SystemLanguage();
    const LocalizedText& text = Text(language);
    const std::string title = result.failure == FailureKind::DriverDenied
        ? text.driverTitle : text.vulkanTitle;
    const std::string details = Details(result, language);

    int selected = Run({"zenity", "--question", "--title=" + title,
                        "--text=" + details, "--ok-label=" + std::string(text.updateDriver),
                        "--cancel-label=" + std::string(text.exit), "--width=520"});
    if (selected == 127) {
        selected = Run({"kdialog", "--title", title, "--yesno", details,
                        "--yes-label", text.updateDriver, "--no-label", text.exit});
    }
    if (selected < 0 || selected == 127) {
        std::fprintf(stderr, "%s\n%s\n", title.c_str(), details.c_str());
        return false;
    }
    if (selected == 0 && !result.downloadUrl.empty()) {
        Run({"xdg-open", result.downloadUrl});
        return true;
    }
    return false;
}

}  // namespace uvdg
