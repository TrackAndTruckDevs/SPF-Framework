#pragma once

#include "SPF/Namespace.hpp"
#include <string>
#include <vector>
#include <optional>
#include <future>

SPF_NS_BEGIN
namespace System {

        /**
         * @brief A simple structure to hold parsed semantic versioning info.
         */
        struct Version {
            int major = 0;
            int minor = 0;
            int patch = 0;
    
            bool operator>(const Version& other) const;
            bool operator==(const Version& other) const;
            static std::optional<Version> FromString(const std::string& versionStr);
        };
    
        /**
         * @brief Holds information about a single patron.
         */
        struct Patron {
            std::string name;
            int tier;
        };
    
        /**
         * @brief A generic wrapper for results from an API call.
         *
         * @tparam T The type of data expected on success.
         */
        template<typename T>
        struct ApiResult {
            bool success = false;
            std::optional<T> data;
            std::optional<std::string> errorMessage; // Localization key for the error
        };
    
        /**
         * @brief Holds information about the latest framework update.
         */
        struct UpdateInfo {
            bool updateAvailable = false;
            std::string status;   // e.g., "update_available", "up_to_date", "switch_to_release"
            std::string severity; // e.g., "major", "minor", "patch"
            Version latestVersion;
            std::string formattedLatestVersion;
            std::string downloadUrl;
            std::string changelog;
        };
    
        /**
         * @brief A service responsible for making remote API calls.
         *
         * This class provides an interface for fetching data from a remote server.
         * The actual implementation will be asynchronous.
         */
        class ApiService {
        public:
            // Asynchronously fetches the latest update information.
            std::future<ApiResult<UpdateInfo>> FetchUpdateInfoAsync(const std::string& baseUrl, int major, int minor, int patch, const std::string& channel);
    
            // Asynchronously fetches the list of patrons.
            std::future<ApiResult<std::vector<Patron>>> FetchPatronsAsync(const std::string& baseUrl);

            // Asynchronously sends anonymous usage data.
            std::future<void> TrackUsageAsync(const std::string& baseUrl, std::string uuid, std::string version);
        };
} // namespace System
SPF_NS_END
