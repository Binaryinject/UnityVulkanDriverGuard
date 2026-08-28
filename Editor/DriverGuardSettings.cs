using System;
using System.Collections.Generic;
using UnityEditor;
using UnityEngine;

namespace UnityVulkanDriverGuard.Editor
{
    internal enum DriverRulePlatform
    {
        All,
        Windows,
        Linux
    }

    [Serializable]
    internal sealed class DriverDenyRule
    {
        public DriverRulePlatform platform = DriverRulePlatform.All;
        public string comparison = "<516.25";
        public string reason = "Known Vulkan driver issue";
    }

    [Serializable]
    internal sealed class VendorDriverPolicy
    {
        public string vendorName;
        public string windowsSuggestedVersion;
        public string linuxSuggestedVersion;
        public string downloadUrl;
        public List<DriverDenyRule> denyList = new List<DriverDenyRule>();
    }

    [FilePath("ProjectSettings/UnityVulkanDriverGuardSettings.asset", FilePathAttribute.Location.ProjectFolder)]
    internal sealed class DriverGuardSettings : ScriptableSingleton<DriverGuardSettings>
    {
        public int minimumVulkanMajor = 1;
        public int minimumVulkanMinor = 1;
        public VendorDriverPolicy nvidia = CreateNvidia();
        public VendorDriverPolicy amd = CreateAmd();
        public VendorDriverPolicy intel = CreateIntel();

        public void SaveSettings()
        {
            Save(true);
        }

        private static VendorDriverPolicy CreateNvidia()
        {
            return new VendorDriverPolicy
            {
                vendorName = "NVIDIA",
                windowsSuggestedVersion = "516.25",
                linuxSuggestedVersion = "515.43.04",
                downloadUrl = "https://www.nvidia.com/Download/index.aspx",
                denyList = new List<DriverDenyRule>
                {
                    new DriverDenyRule { platform = DriverRulePlatform.Windows, comparison = "<516.25" },
                    new DriverDenyRule { platform = DriverRulePlatform.Linux, comparison = "<515.43.04" }
                }
            };
        }

        private static VendorDriverPolicy CreateAmd()
        {
            return new VendorDriverPolicy
            {
                vendorName = "AMD",
                windowsSuggestedVersion = "22.5.1",
                linuxSuggestedVersion = "22.0.0",
                downloadUrl = "https://www.amd.com/en/support/download/drivers.html",
                denyList = new List<DriverDenyRule>
                {
                    new DriverDenyRule { platform = DriverRulePlatform.Windows, comparison = "<22.5.1" },
                    new DriverDenyRule { platform = DriverRulePlatform.Linux, comparison = "<22.0.0" }
                }
            };
        }

        private static VendorDriverPolicy CreateIntel()
        {
            return new VendorDriverPolicy
            {
                vendorName = "Intel",
                windowsSuggestedVersion = "101.3413",
                linuxSuggestedVersion = "22.0.0",
                downloadUrl = "https://www.intel.com/content/www/us/en/download-center/home.html",
                denyList = new List<DriverDenyRule>
                {
                    new DriverDenyRule { platform = DriverRulePlatform.Windows, comparison = "<101.3413" },
                    new DriverDenyRule { platform = DriverRulePlatform.Linux, comparison = "<22.0.0" }
                }
            };
        }
    }

    internal sealed class DriverGuardSettingsProvider : SettingsProvider
    {
        private UnityEditor.Editor settingsEditor;

        private DriverGuardSettingsProvider()
            : base("Project/Player/Unity Vulkan Driver Guard", SettingsScope.Project)
        {
            keywords = new HashSet<string>(new[] { "Vulkan", "GPU", "Driver", "Deny List" });
        }

        [SettingsProvider]
        public static SettingsProvider Create()
        {
            return new DriverGuardSettingsProvider();
        }

        public override void OnGUI(string searchContext)
        {
            var settings = DriverGuardSettings.instance;
            if (settingsEditor == null)
                UnityEditor.Editor.CreateCachedEditor(settings, null, ref settingsEditor);

            EditorGUI.BeginChangeCheck();
            settingsEditor.OnInspectorGUI();
            if (EditorGUI.EndChangeCheck())
                settings.SaveSettings();
        }
    }
}

