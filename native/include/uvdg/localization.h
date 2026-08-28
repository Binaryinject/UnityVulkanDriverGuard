#pragma once

#include "uvdg/types.h"

#include <string>

namespace uvdg {

enum class Language {
    English,
    Chinese,
    Japanese,
    Korean
};

struct LocalizedText {
    const char* windowTitle;
    const char* driverTitle;
    const char* vulkanTitle;
    const char* updateDriver;
    const char* continueRunning;
    const char* exit;
    const char* gpu;
    const char* installedDriver;
    const char* vulkanApi;
    const char* requiredVulkanApi;
    const char* orNewer;
    const char* recommendedDriver;
};

Language LanguageFromLocale(const std::string& locale);
const LocalizedText& Text(Language language);
std::string LocalizedReason(Language language, const PreflightResult& result);

}  // namespace uvdg
