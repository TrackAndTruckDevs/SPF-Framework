#include "SPF/Modules/API/ManifestApi.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/SPF_API/SPF_Manifest_API.h" // Builder definitions

#include <nlohmann/json.hpp>

SPF_NS_BEGIN
namespace Modules::API {

using namespace SPF::Config;

// =================================================================================================
// 1. Helper Functions (Internal)
// =================================================================================================

static ManifestData* Cast(SPF_Manifest_Builder_Handle* h) {
    return reinterpret_cast<ManifestData*>(h);
}

static void LogError(const char* context, const char* message) {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ManifestApi");
    if (logger) logger->Error("{}: {}", context, message);
}

// =================================================================================================
// 2. Info Implementations
// =================================================================================================

static void Info_SetName(SPF_Manifest_Builder_Handle* h, const char* name) {
    if (h && name) Cast(h)->info.name = name;
}

static void Info_SetVersion(SPF_Manifest_Builder_Handle* h, const char* version) {
    if (h && version) Cast(h)->info.version = version;
}

static void Info_SetMinFrameworkVersion(SPF_Manifest_Builder_Handle* h, const char* version) {
    if (h && version) Cast(h)->info.minFrameworkVersion = version;
}

static void Info_SetAuthor(SPF_Manifest_Builder_Handle* h, const char* author) {
    if (h && author) Cast(h)->info.author = author;
}

static void Info_SetDescriptionKey(SPF_Manifest_Builder_Handle* h, const char* key) {
    if (h && key) Cast(h)->info.descriptionKey = key;
}

static void Info_SetDescriptionLiteral(SPF_Manifest_Builder_Handle* h, const char* desc) {
    if (h && desc) Cast(h)->info.descriptionLiteral = desc;
}

static void Info_SetEmail(SPF_Manifest_Builder_Handle* h, const char* email) {
    if (h && email) Cast(h)->info.email = email;
}

static void Info_SetDiscordUrl(SPF_Manifest_Builder_Handle* h, const char* url) {
    if (h && url) Cast(h)->info.discordUrl = url;
}

static void Info_SetSteamProfileUrl(SPF_Manifest_Builder_Handle* h, const char* url) {
    if (h && url) Cast(h)->info.steamProfileUrl = url;
}

static void Info_SetGithubUrl(SPF_Manifest_Builder_Handle* h, const char* url) {
    if (h && url) Cast(h)->info.githubUrl = url;
}

static void Info_SetYoutubeUrl(SPF_Manifest_Builder_Handle* h, const char* url) {
    if (h && url) Cast(h)->info.youtubeUrl = url;
}

static void Info_SetScsForumUrl(SPF_Manifest_Builder_Handle* h, const char* url) {
    if (h && url) Cast(h)->info.scsForumUrl = url;
}

static void Info_SetPatreonUrl(SPF_Manifest_Builder_Handle* h, const char* url) {
    if (h && url) Cast(h)->info.patreonUrl = url;
}

static void Info_SetWebsiteUrl(SPF_Manifest_Builder_Handle* h, const char* url) {
    if (h && url) Cast(h)->info.websiteUrl = url;
}

// =================================================================================================
// 3. Policy Implementations
// =================================================================================================

static void Policy_SetAllowUserConfig(SPF_Manifest_Builder_Handle* h, bool allow) {
    if (h) Cast(h)->configPolicy.allowUserConfig = allow;
}

static void Policy_AddConfigurableSystem(SPF_Manifest_Builder_Handle* h, const char* systemName) {
    if (h && systemName) Cast(h)->configPolicy.userConfigurableSystems.push_back(systemName);
}

static void Policy_AddRequiredHook(SPF_Manifest_Builder_Handle* h, const char* hookName) {
    if (h && hookName) Cast(h)->configPolicy.requiredHooks.push_back(hookName);
}

// =================================================================================================
// 4. Settings Defaults (JSON)
// =================================================================================================

static void Settings_SetJson(SPF_Manifest_Builder_Handle* h, const char* jsonStr) {
    if (!h || !jsonStr || *jsonStr == '\0') return;
    try {
        auto j = nlohmann::ordered_json::parse(jsonStr);
        if (j.is_object()) {
            Cast(h)->settings = std::move(j);
        } else {
            LogError("Settings_SetJson", "Root of custom settings must be a JSON object.");
        }
    } catch (const std::exception& e) {
        LogError("Settings_SetJson", e.what());
    }
}

// =================================================================================================
// 5. System Defaults
// =================================================================================================

static void Defaults_SetLogging(SPF_Manifest_Builder_Handle* h, const char* level, bool fileSink) {
    if (!h) return;
    auto* m = Cast(h);
    if (level && *level) m->logging.level = level;
    m->logging.sinks.file = fileSink;
    m->logging.sinks.ui = true; // Always enable UI logging by default, controlled by framework
}

static void Defaults_SetLocalization(SPF_Manifest_Builder_Handle* h, const char* langCode) {
    if (h && langCode && *langCode) Cast(h)->localization.language = langCode;
}

static void Defaults_AddKeybind(SPF_Manifest_Builder_Handle* h, const char* group, const char* action, 
                                const char* type, const char* key, const char* pressType, 
                                int thresholdMs, const char* consume, const char* behavior) {
    if (!h || !group || !action || *group == '\0' || *action == '\0') return;
    
    KeybindDefinition def;
    if (type && *type) def.type = type;
    if (key && *key) def.key = key;
    if (pressType && *pressType) def.pressType = pressType;
    if (thresholdMs > 0) def.pressThresholdMs = thresholdMs;
    if (consume && *consume) def.consume = consume;
    if (behavior && *behavior) def.behavior = behavior;

    Cast(h)->keybinds.actions[group][action].push_back(std::move(def));
}

static void Defaults_AddWindow(SPF_Manifest_Builder_Handle* h, const char* windowName, 
                               bool isVisible, bool isInteractive, 
                               int x, int y, int w, int height, 
                               bool isCollapsed, bool autoScroll) {
    if (!h || !windowName || *windowName == '\0') return;

    WindowData win;
    win.isVisible = isVisible;
    win.isInteractive = isInteractive;
    win.posX = x;
    win.posY = y;
    win.sizeW = w;
    win.sizeH = height;
    win.isCollapsed = isCollapsed;
    win.autoScroll = autoScroll;

    Cast(h)->ui.windows[windowName] = win;
}

// =================================================================================================
// 6. Metadata Implementations
// =================================================================================================

static void Meta_AddCustomSetting(SPF_Manifest_Builder_Handle* h, const char* keyPath, 
                                  const char* titleKey, const char* descKey, 
                                  const char* widgetType, const char* widgetParamsJson, bool hideInUI) {
    if (!h || !keyPath || *keyPath == '\0') return;

    CustomSettingMetadata meta;
    meta.keyPath = keyPath;
    if (titleKey && *titleKey) meta.titleKey = titleKey;
    if (descKey && *descKey) meta.descriptionKey = descKey;
    meta.hide_in_ui = hideInUI;

    if (widgetType && *widgetType) meta.widget = widgetType;
    if (widgetParamsJson && *widgetParamsJson) {
        try {
            auto j = nlohmann::ordered_json::parse(widgetParamsJson);
            if (j.is_object()) {
                meta.widget_params = std::move(j);
            } else {
                LogError("Meta_AddCustomSetting", "Widget parameters must be a JSON object.");
            }
        } catch (const std::exception& e) {
            LogError("Meta_AddCustomSetting", e.what());
        }
    }

    Cast(h)->customSettingsMetadata.push_back(std::move(meta));
}

static void Meta_AddKeybind(SPF_Manifest_Builder_Handle* h, const char* group, const char* action, const char* title, const char* desc) {
    if (!h || !group || !action) return;
    KeybindActionMetadata meta;
    meta.groupName = group;
    meta.actionName = action;
    if (title) meta.titleKey = title;
    if (desc) meta.descriptionKey = desc;
    Cast(h)->keybindsMetadata.push_back(meta);
}

static void Meta_AddWindow(SPF_Manifest_Builder_Handle* h, const char* windowName, const char* title, const char* desc) {
    if (!h || !windowName) return;
    WindowMetadata meta;
    meta.windowName = windowName;
    if (title) meta.titleKey = title;
    if (desc) meta.descriptionKey = desc;
    Cast(h)->uiMetadata.push_back(meta);
}

static void Meta_AddStandardSetting(SPF_Manifest_Builder_Handle* h, const char* system, const char* key, const char* title, const char* desc) {
    if (!h || !system || !key) return;
    
    StandardSettingMetadata meta;
    meta.key = key;
    if (title) meta.titleKey = title;
    if (desc) meta.descriptionKey = desc;

    if (std::string(system) == "logging") {
        Cast(h)->loggingMetadata.push_back(meta);
    } else if (std::string(system) == "localization") {
        Cast(h)->localizationMetadata.push_back(meta);
    }
}

// =================================================================================================
// 7. API Table Population
// =================================================================================================

static void FillBuilderApi(SPF_Manifest_Builder_API* api) {
    api->Info_SetName = Info_SetName;
    api->Info_SetVersion = Info_SetVersion;
    api->Info_SetMinFrameworkVersion = Info_SetMinFrameworkVersion;
    api->Info_SetAuthor = Info_SetAuthor;
    api->Info_SetDescriptionKey = Info_SetDescriptionKey;
    api->Info_SetDescriptionLiteral = Info_SetDescriptionLiteral;
    api->Info_SetEmail = Info_SetEmail;
    api->Info_SetDiscordUrl = Info_SetDiscordUrl;
    api->Info_SetSteamProfileUrl = Info_SetSteamProfileUrl;
    api->Info_SetGithubUrl = Info_SetGithubUrl;
    api->Info_SetYoutubeUrl = Info_SetYoutubeUrl;
    api->Info_SetScsForumUrl = Info_SetScsForumUrl;
    api->Info_SetPatreonUrl = Info_SetPatreonUrl;
    api->Info_SetWebsiteUrl = Info_SetWebsiteUrl;

    api->Policy_SetAllowUserConfig = Policy_SetAllowUserConfig;
    api->Policy_AddConfigurableSystem = Policy_AddConfigurableSystem;
    api->Policy_AddRequiredHook = Policy_AddRequiredHook;

    api->Settings_SetJson = Settings_SetJson;

    api->Defaults_SetLogging = Defaults_SetLogging;
    api->Defaults_SetLocalization = Defaults_SetLocalization;
    api->Defaults_AddKeybind = Defaults_AddKeybind;
    api->Defaults_AddWindow = Defaults_AddWindow;

    api->Meta_AddCustomSetting = Meta_AddCustomSetting;
    api->Meta_AddKeybind = Meta_AddKeybind;
    api->Meta_AddWindow = Meta_AddWindow;
    api->Meta_AddStandardSetting = Meta_AddStandardSetting;
}

// =================================================================================================
// 8. Main Entry Point
// =================================================================================================

ManifestData ManifestApi::BuildManifest(SPF_GetManifestAPI_Func pGetManifestFunc, const std::string& pluginName) {
    ManifestData manifest;
    
    // Default name fallback
    manifest.info.name = pluginName;

    if (!pGetManifestFunc) {
        return manifest;
    }

    SPF_Manifest_API pluginApi;
    if (!pGetManifestFunc(&pluginApi) || !pluginApi.BuildManifest) {
        return manifest;
    }

    // Create the Builder API table
    SPF_Manifest_Builder_API builderApi;
    FillBuilderApi(&builderApi);

    // Call the plugin's builder function
    // cast &manifest to the opaque handle type
    try {
        pluginApi.BuildManifest(reinterpret_cast<SPF_Manifest_Builder_Handle*>(&manifest), &builderApi);
    } catch (const std::exception& e) {
        LogError("BuildManifest", e.what());
    } catch (...) {
        LogError("BuildManifest", "Unknown exception occurred in plugin's BuildManifest.");
    }

    return manifest;
}

} // namespace Modules::API
SPF_NS_END
