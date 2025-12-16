#pragma once

#include "SPF/Namespace.hpp"
#include <string>
#include <vector>
#include <optional>

SPF_NS_BEGIN

namespace Config {
/**
 * @brief A structure to hold all relevant information about a component (framework or plugin) for display and management.
 *
 * This structure is populated by the ConfigService by combining data from a component's
 * manifest and the framework's user settings.
 */
struct ComponentInfo {
    // --- Static Info from Manifest ---
    std::optional<std::string> name;                ///< @brief Display name of the plugin/component.
    std::optional<std::string> version;             ///< @brief Version string of the plugin.
    std::optional<std::string> author;              ///< @brief Author of the plugin.
    std::optional<std::string> descriptionKey;      ///< @brief Localization key for the plugin's description.
    std::optional<std::string> descriptionLiteral;  ///< @brief Literal description if no localization key is provided.
    
    // Social and Contact Info
    std::optional<std::string> email;               ///< @brief Author's contact email.
    std::optional<std::string> discordUrl;          ///< @brief URL to a Discord server invite.  
    std::optional<std::string> steamProfileUrl;     ///< @brief URL to a Steam profile.     
    std::optional<std::string> githubUrl;           ///< @brief URL to the GitHub repository.
    std::optional<std::string> youtubeUrl;          ///< @brief URL to a YouTube channel/video.
    std::optional<std::string> scsForumUrl;         ///< @brief URL to an SCS Software forum thread.
    std::optional<std::string> patreonUrl;          ///< @brief URL to a Patreon page.   
    std::optional<std::string> websiteUrl;          ///< @brief URL to a personal or project website.

    // --- Configuration Policy ---
    bool allowUserConfig = false;                  ///< @brief True if the user can override settings for this component.
    std::vector<std::string> configurableSystems;  ///< @brief List of configuration systems this component exposes to the settings UI.
    std::vector<std::string> required_hooks;       ///< @brief List of hooks required by this plugin for its features to work.

    // --- Dynamic State ---
    bool isFramework = false;  ///< @brief True if this component is the core framework.
    bool isEnabled = false;    ///< @brief True if the plugin is currently loaded/active.
    bool hasSettings = false;  ///< @brief True if this plugin has any settings visible in the UI.
    bool hasDescription = false; ///< @brief True if a description (key or literal) is available.
    bool hasInfo = false;      ///< @brief True if basic info (name, author, version) is available.
};
}  // namespace Config

SPF_NS_END
