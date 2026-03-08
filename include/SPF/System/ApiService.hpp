#pragma once

#include "SPF/Namespace.hpp"
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <future>
#include <mutex>
#include <condition_variable>

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
            bool operator<(const Version& other) const;
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
         * @brief Represents a single report entry for an error or warning for remote reporting.
         */
        struct LogReportEntry {
            std::string loggerName;
            std::string level;
            std::string message;
            uint32_t count;
        };

        /**
         * @brief Holds markdown changelog data for a specific version.
         */
        struct ChangelogData {
            std::string title;
            std::string markdown;
        };

        /**
         * @brief Holds information about the latest framework update.
         */
        struct UpdateInfo {
            bool updateAvailable = false;
            struct {
                Version ver;
                std::string full;
            } latestVersion;
            std::string downloadUrl;
            struct {
                std::string archive;
                std::string binary;
            } md5;
            ChangelogData content;
        };
    
        /**
         * @brief Represents the current state of the API service connectivity.
         */
        enum class ServiceStatus {
            Unknown,
            Online,
            Offline,
            Banned,
            ServerError
        };

        /**
         * @brief A service responsible for making remote API calls.
         *
         * This class provides an interface for fetching data from a remote server.
         * The actual implementation will be asynchronous.
         */
        class ApiService {
        public:
            ApiService();
            ~ApiService() = default;

            // Asynchronously fetches the latest update information using the new get_framework_update.php.
            std::future<ApiResult<UpdateInfo>> FetchUpdateInfoAsync(const std::string& baseUrl, int major, int minor, int patch, const std::string& channel, const std::string& lang);
    
            // Asynchronously fetches localized release notes for a specific version using get_release_notes.php.
            std::future<ApiResult<ChangelogData>> FetchReleaseNotesAsync(const std::string& baseUrl, int major, int minor, int patch, const std::string& lang);

            // Asynchronously fetches the list of patrons.
            std::future<ApiResult<std::vector<Patron>>> FetchPatronsAsync(const std::string& baseUrl);

            // Asynchronously sends anonymous usage data and grouped logs.
            std::future<void> TrackUsageAsync(const std::string& baseUrl, std::string uuid, std::string sessionId, std::string buildHash, std::string version, std::string game, std::string gameVersion, std::map<std::string, bool> plugins, std::vector<LogReportEntry> logs);

            /**
             * @brief Gets the last known status of the service.
             */
            ServiceStatus GetLastStatus() const;

        private:
            struct ConnectivityState {
                ServiceStatus status = ServiceStatus::Unknown;
                std::string lastErrorMessage;
                std::chrono::steady_clock::time_point lastCheckTime;
                bool isChecking = false;
            };

            ConnectivityState m_state;
            mutable std::mutex m_stateMutex;
            std::condition_variable m_connectivityCV;

            /**
             * @brief Performs a health check if needed (based on time threshold).
             * This is called internally by all public API methods.
             * @return true if service is Online, false otherwise.
             */
            bool EnsureConnectivity(const std::string& baseUrl);
        };
} // namespace System
SPF_NS_END
