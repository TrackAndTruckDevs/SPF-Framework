#pragma once

#include "SPF/Namespace.hpp"
#include "SPF/System/ApiService.hpp"

SPF_NS_BEGIN
namespace Events::System {

/**
 * @brief Fired by the UpdateManager when an update check succeeds.
 */
struct OnUpdateCheckSucceeded {
    const SPF::System::UpdateInfo& updateInfo;
};

/**
 * @brief Fired by the UpdateManager when an update check fails.
 */
struct OnUpdateCheckFailed {
    const std::optional<std::string>& errorMessage;
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

}  // namespace Events::System
SPF_NS_END
