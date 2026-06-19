#pragma once

#include "SPF/Namespace.hpp"
#include "SPF/System/ApiService.hpp"

SPF_NS_BEGIN
namespace Events::System {

/**
 * @brief Fired by the CommunicationManager when an update check completes (success or failure).
 */
struct OnUpdateCheckCompleted {
    const SPF::System::ApiResult<SPF::System::UpdateInfo>& result;
};

/**
 * @brief Fired by the Core to request that usage be tracked for the current session.
 */
struct OnRequestTrackUsage {};

/**
 * @brief Fired after the patrons list has been fetched from the API.
 */
struct OnPatronsFetchCompleted {
    const SPF::System::ApiResult<std::vector<SPF::System::Patron>>& result;
};

/**
 * @brief Fired after analytics session has been attempted.
 */
struct OnUsageTrackingCompleted {
    bool success;
};

/**
 * @brief Fired when an update for a plugin is detected via GitHub.
 */
struct OnPluginUpdateAvailable {
    std::string pluginId;
    std::string pluginName;
    std::string currentVersion;
    std::string latestVersion;
    std::string downloadUrl;
};

}  // namespace Events::System
SPF_NS_END
