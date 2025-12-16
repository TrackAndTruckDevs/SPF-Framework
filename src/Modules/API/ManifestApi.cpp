// src/Modules/API/ManifestApi.cpp
#include "SPF/Modules/API/ManifestApi.hpp"
#include "SPF/Logging/LoggerFactory.hpp" // For logging
#include "fmt/format.h"

#include <string> // For std::string
#include <nlohmann/json.hpp> // For nlohmann::json
#include <cstring> // For strncpy_s
#include <algorithm> // For std::min

SPF_NS_BEGIN

namespace Modules::API {

SPF::Config::ManifestData ManifestApi::ConvertCManifestToCppManifest(const SPF_ManifestData_C& cManifest, const std::string& pluginName) {
    SPF::Config::ManifestData cppManifest;
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ManifestApi");

    // --- Info Data ---
    if (cManifest.info.name[0] != '\0') cppManifest.info.name = cManifest.info.name;
    if (cManifest.info.version[0] != '\0') cppManifest.info.version = cManifest.info.version;
    if (cManifest.info.author[0] != '\0') cppManifest.info.author = cManifest.info.author;
    if (cManifest.info.descriptionKey[0] != '\0') cppManifest.info.descriptionKey = cManifest.info.descriptionKey;
    if (cManifest.info.descriptionLiteral[0] != '\0') cppManifest.info.descriptionLiteral = cManifest.info.descriptionLiteral;
    if (cManifest.info.email[0] != '\0') cppManifest.info.email = cManifest.info.email;
    if (cManifest.info.discordUrl[0] != '\0') cppManifest.info.discordUrl = cManifest.info.discordUrl;
    if (cManifest.info.steamProfileUrl[0] != '\0') cppManifest.info.steamProfileUrl = cManifest.info.steamProfileUrl;
    if (cManifest.info.githubUrl[0] != '\0') cppManifest.info.githubUrl = cManifest.info.githubUrl;
    if (cManifest.info.youtubeUrl[0] != '\0') cppManifest.info.youtubeUrl = cManifest.info.youtubeUrl;
    if (cManifest.info.scsForumUrl[0] != '\0') cppManifest.info.scsForumUrl = cManifest.info.scsForumUrl;
    if (cManifest.info.patreonUrl[0] != '\0') cppManifest.info.patreonUrl = cManifest.info.patreonUrl;
    if (cManifest.info.websiteUrl[0] != '\0') cppManifest.info.websiteUrl = cManifest.info.websiteUrl;

    // --- Config Policy Data ---
    cppManifest.configPolicy.allowUserConfig = cManifest.configPolicy.allowUserConfig;
    for (unsigned int i = 0; i < cManifest.configPolicy.userConfigurableSystemsCount && i < SPF_MANIFEST_MAX_SYSTEMS; ++i) {
        cppManifest.configPolicy.userConfigurableSystems.push_back(cManifest.configPolicy.userConfigurableSystems[i]);
    }
    for (unsigned int i = 0; i < cManifest.configPolicy.requiredHooksCount && i < SPF_MANIFEST_MAX_HOOKS; ++i) {
        cppManifest.configPolicy.requiredHooks.push_back(cManifest.configPolicy.requiredHooks[i]);
    }

    // --- Settings Data (JSON string to nlohmann::json) ---
    if (cManifest.settingsJson) {
        try {
            cppManifest.settings = nlohmann::json::parse(cManifest.settingsJson);
        } catch (const nlohmann::json::parse_error& e) {
            logger->Error("ConvertCManifestToCppManifest: Failed to parse settings JSON for plugin '{}'. Error: {}. Returning empty settings.", pluginName, e.what());
            cppManifest.settings = nlohmann::json::object();
        }
    } else {
        cppManifest.settings = nlohmann::json::object();
    }

    // --- Logging Data ---
    if (cManifest.logging.level[0] != '\0') {
        cppManifest.logging.level = cManifest.logging.level;
        cppManifest.logging.sinks.file = cManifest.logging.sinks.file;
        // cppManifest.logging.sinks.ui = cManifest.logging.sinks.ui; // This line is handled by the user
    }

    // --- Localization Data ---
    if (cManifest.localization.language[0] != '\0') {
        cppManifest.localization.language = cManifest.localization.language;
    }

    // --- Keybinds Data ---
    for (unsigned int i = 0; i < cManifest.keybinds.actionCount && i < SPF_MANIFEST_MAX_ACTIONS_PER_GROUP; ++i) {
        const auto& cAction = cManifest.keybinds.actions[i];
        std::string groupName = cAction.groupName;
        std::string actionName = cAction.actionName;

        if (groupName.empty() || actionName.empty()) continue;

        for (unsigned int j = 0; j < cAction.definitionCount && j < SPF_MANIFEST_MAX_KEYBINDS_PER_ACTION; ++j) {
            const auto& cDef = cAction.definitions[j];
            SPF::Config::KeybindDefinition def_cpp;
            if (cDef.type[0] != '\0') def_cpp.type = cDef.type;
            if (cDef.key[0] != '\0') def_cpp.key = cDef.key;
            if (cDef.pressType[0] != '\0') def_cpp.pressType = cDef.pressType;
            if (cDef.pressThresholdMs != 0) def_cpp.pressThresholdMs = cDef.pressThresholdMs;
            if (cDef.consume[0] != '\0') def_cpp.consume = cDef.consume;
            if (cDef.behavior[0] != '\0') def_cpp.behavior = cDef.behavior;
            cppManifest.keybinds.actions[groupName][actionName].push_back(def_cpp);
        }
    }

    // --- UI Data ---
    for (unsigned int i = 0; i < cManifest.ui.windowsCount && i < SPF_MANIFEST_MAX_WINDOWS; ++i) {
        const auto& cWindow = cManifest.ui.windows[i];
        std::string windowName = cWindow.name;
        if (windowName.empty()) {
            windowName = fmt::format("UnnamedWindow_{}", i);
            logger->Warn("ConvertCManifestToCppManifest: Window at index {} for plugin '{}' has no name. Using a generated name: '{}'.", i, pluginName, windowName);
        }

        SPF::Config::WindowData windowData;
        windowData.isVisible = cWindow.isVisible;
        windowData.isInteractive = cWindow.isInteractive;
        if (cWindow.posX != 0) windowData.posX = cWindow.posX;
        if (cWindow.posY != 0) windowData.posY = cWindow.posY;
        if (cWindow.sizeW != 0) windowData.sizeW = cWindow.sizeW;
        if (cWindow.sizeH != 0) windowData.sizeH = cWindow.sizeH;
        windowData.isCollapsed = cWindow.isCollapsed;
        // windowData.isDocked = cWindow.isDocked;
        // windowData.dockPriority = cWindow.dockPriority;
        // windowData.allowUndocking = cWindow.allowUndocking;
        windowData.autoScroll = cWindow.autoScroll;
        
        cppManifest.ui.windows[windowName] = windowData;
    }

    // --- Custom Settings Metadata ---
    for (unsigned int i = 0; i < cManifest.customSettingsMetadataCount && i < 128; ++i) {
        const auto& cMeta = cManifest.customSettingsMetadata[i];
        if (cMeta.keyPath[0] == '\0') continue;

        SPF::Config::CustomSettingMetadata cppMeta;
        cppMeta.keyPath = cMeta.keyPath;
        if (cMeta.titleKey[0] != '\0') cppMeta.titleKey = cMeta.titleKey;
        if (cMeta.descriptionKey[0] != '\0') cppMeta.descriptionKey = cMeta.descriptionKey;
        cppManifest.customSettingsMetadata.push_back(cppMeta);
    }

    // --- Keybind Action Metadata ---
    for (unsigned int i = 0; i < cManifest.keybindsMetadataCount && i < 128; ++i) {
        const auto& cMeta = cManifest.keybindsMetadata[i];
        if (cMeta.groupName[0] == '\0' || cMeta.actionName[0] == '\0') continue;

        SPF::Config::KeybindActionMetadata cppMeta;
        cppMeta.groupName = cMeta.groupName;
        cppMeta.actionName = cMeta.actionName;
        if (cMeta.titleKey[0] != '\0') cppMeta.titleKey = cMeta.titleKey;
        if (cMeta.descriptionKey[0] != '\0') cppMeta.descriptionKey = cMeta.descriptionKey;
        cppManifest.keybindsMetadata.push_back(cppMeta);
    }

    // --- Logging Metadata ---
    for (unsigned int i = 0; i < cManifest.loggingMetadataCount && i < 16; ++i) {
        const auto& cMeta = cManifest.loggingMetadata[i];
        if (cMeta.key[0] == '\0') continue;

        SPF::Config::StandardSettingMetadata cppMeta;
        cppMeta.key = cMeta.key;
        if (cMeta.titleKey[0] != '\0') cppMeta.titleKey = cMeta.titleKey;
        if (cMeta.descriptionKey[0] != '\0') cppMeta.descriptionKey = cMeta.descriptionKey;
        cppManifest.loggingMetadata.push_back(cppMeta);
    }

    // --- Localization Metadata ---
    for (unsigned int i = 0; i < cManifest.localizationMetadataCount && i < 16; ++i) {
        const auto& cMeta = cManifest.localizationMetadata[i];
        if (cMeta.key[0] == '\0') continue;

        SPF::Config::StandardSettingMetadata cppMeta;
        cppMeta.key = cMeta.key;
        if (cMeta.titleKey[0] != '\0') cppMeta.titleKey = cMeta.titleKey;
        if (cMeta.descriptionKey[0] != '\0') cppMeta.descriptionKey = cMeta.descriptionKey;
        cppManifest.localizationMetadata.push_back(cppMeta);
    }

    // --- UI Metadata ---
    for (unsigned int i = 0; i < cManifest.uiMetadataCount && i < SPF_MANIFEST_MAX_WINDOWS; ++i) {
        const auto& cMeta = cManifest.uiMetadata[i];
        if (cMeta.windowName[0] == '\0') continue;

        SPF::Config::WindowMetadata cppMeta;
        cppMeta.windowName = cMeta.windowName;
        if (cMeta.titleKey[0] != '\0') cppMeta.titleKey = cMeta.titleKey;
        if (cMeta.descriptionKey[0] != '\0') cppMeta.descriptionKey = cMeta.descriptionKey;
        cppManifest.uiMetadata.push_back(cppMeta);
    }

    return cppManifest;
}

} // namespace Modules::API

SPF_NS_END