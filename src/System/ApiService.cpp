#include "SPF/System/ApiService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <cpr/cpr.h> // Include cpr
#include <nlohmann/json.hpp>
#include <fmt/format.h>

#include <chrono>
#include <thread>
#include <random> // For mock error simulation

// Use the nlohmann::ordered_json library
using json = nlohmann::ordered_json;

namespace {
    // --- Constants for the API ---
    constexpr const char* API_HEALTH_PATH = "/api/v1/health.php";
    constexpr const char* API_UPDATE_PATH = "/api/v1/check_update.php";
    constexpr const char* API_PATRONS_PATH = "/api/v1/get_patrons.php";
    constexpr const char* API_TRACK_USAGE_PATH = "/api/v1/track_usage.php";
    
    constexpr const char* API_CLIENT_SECRET = "SPF_API_SEC_6ccfd2c1-7b9d-48e1-9f0a-3d2e1c0b9a8f_TRUCKSIMHUB";

    constexpr auto HEALTH_CHECK_INTERVAL = std::chrono::minutes(5);
}

SPF_NS_BEGIN
namespace System {

    // --- ApiService Implementation ---

    ApiService::ApiService() {
        m_state.lastCheckTime = std::chrono::steady_clock::now() - HEALTH_CHECK_INTERVAL - std::chrono::seconds(1);
    }

    ServiceStatus ApiService::GetLastStatus() const {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        return m_state.status;
    }

    bool ApiService::EnsureConnectivity(const std::string& baseUrl) {
        auto now = std::chrono::steady_clock::now();
        
        std::unique_lock<std::mutex> lock(m_stateMutex);

        // 1. If another thread is already checking, wait for it
        if (m_state.isChecking) {
            m_connectivityCV.wait(lock, [this] { return !m_state.isChecking; });
            // After waiting, we should have a fresh status. 
            // Re-check time to see if we can just return the result.
            if (std::chrono::steady_clock::now() - m_state.lastCheckTime < HEALTH_CHECK_INTERVAL) {
                return m_state.status == ServiceStatus::Online;
            }
        }

        // 2. Check if we have a recent valid status
        if (now - m_state.lastCheckTime < HEALTH_CHECK_INTERVAL && m_state.status != ServiceStatus::Unknown) {
            return m_state.status == ServiceStatus::Online;
        }

        // 3. Start a new check
        m_state.isChecking = true;
        lock.unlock(); // Unlock while performing network I/O

        auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ApiService");

        try {
            cpr::Response r = cpr::Post(cpr::Url{baseUrl + API_HEALTH_PATH},
                                        cpr::Header{{"X-API-Key", API_CLIENT_SECRET}},
                                        cpr::Timeout{5000},
                                        cpr::ConnectTimeout{3000});

            ServiceStatus newStatus;
            std::string errorMsg;

            if (r.error.code != cpr::ErrorCode::OK) {
                newStatus = ServiceStatus::Offline;
                errorMsg = "api.error.no_internet";
            } else if (r.status_code == 200) {
                newStatus = ServiceStatus::Online;
                errorMsg = "";
            } else if (r.status_code == 403) {
                newStatus = ServiceStatus::Banned;
                errorMsg = "api.error.forbidden";
            } else {
                newStatus = ServiceStatus::ServerError;
                errorMsg = "api.error.server_unavailable";
            }

            lock.lock(); // Re-lock to update state
            
            if (newStatus != m_state.status) {
                if (newStatus == ServiceStatus::Online) {
                    logger->Info("API Service is Online. [BaseUrl: {}]", baseUrl);
                } else {
                    logger->Warn("API Service state changed: {} [HTTP: {}, Error: {}]", 
                        (newStatus == ServiceStatus::Banned ? "BANNED" : "OFFLINE/ERROR"), 
                        r.status_code, r.error.message);
                }
            }

            m_state.status = newStatus;
            m_state.lastErrorMessage = errorMsg;
            m_state.lastCheckTime = std::chrono::steady_clock::now();
            m_state.isChecking = false;
            
            lock.unlock();
            m_connectivityCV.notify_all(); // Wake up everyone waiting
            
            return newStatus == ServiceStatus::Online;

        } catch (const std::exception& e) {
            logger->Error("Unexpected error during health check: {}", e.what());
            
            lock.lock();
            m_state.isChecking = false;
            m_state.lastCheckTime = std::chrono::steady_clock::now(); // Prevent immediate retry
            lock.unlock();
            m_connectivityCV.notify_all();
            
            return false;
        }
    }

    // --- Version Implementation ---
    bool Version::operator>(const Version& other) const {
        if (major != other.major) return major > other.major;
        if (minor != other.minor) return minor > other.minor;
        return patch > other.patch;
    }

    bool Version::operator<(const Version& other) const {
        if (major != other.major) return major < other.major;
        if (minor != other.minor) return minor < other.minor;
        return patch < other.patch;
    }

    bool Version::operator==(const Version& other) const {
        return major == other.major && minor == other.minor && patch == other.patch;
    }

    // More robust implementation of FromString
    std::optional<Version> Version::FromString(const std::string& versionStr) {
        Version v;
        // This will parse the beginning of the string for "X.Y.Z" and ignore any suffixes like "-beta", ".123", etc.
        if (sscanf_s(versionStr.c_str(), "%d.%d.%d", &v.major, &v.minor, &v.patch) >= 3) {
            return v;
        }
        // Attempt to parse just major.minor if patch is missing
        if (sscanf_s(versionStr.c_str(), "%d.%d", &v.major, &v.minor) >= 2) {
            v.patch = 0;
            return v;
        }
        // Attempt to parse just major if minor/patch are missing
        if (sscanf_s(versionStr.c_str(), "%d", &v.major) >= 1) {
            v.minor = 0;
            v.patch = 0;
            return v;
        }
        return std::nullopt;
    }

    // --- ApiService Implementation ---
    std::future<ApiResult<UpdateInfo>> ApiService::FetchUpdateInfoAsync(const std::string& baseUrl, int major, int minor, int patch, const std::string& channel) {
        auto promise = std::make_shared<std::promise<ApiResult<UpdateInfo>>>();
        std::future<ApiResult<UpdateInfo>> future = promise->get_future();

        std::thread([this, promise, baseUrl, major, minor, patch, channel]() {
            auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ApiService");
            ApiResult<UpdateInfo> apiResult;
            
            try {
                // 1. Ensure connectivity before proceeding
                if (!EnsureConnectivity(baseUrl)) {
                    std::lock_guard<std::mutex> lock(m_stateMutex);
                    apiResult.success = false;
                    apiResult.errorMessage = m_state.lastErrorMessage;
                    promise->set_value(apiResult);
                    return;
                }

                logger->Debug("Fetching update info...");

                json requestBody = {
                    {"major", major},
                    {"minor", minor},
                    {"patch", patch},
                    {"channel", channel}
                };

                cpr::Response r = cpr::Post(cpr::Url{baseUrl + API_UPDATE_PATH},
                                            cpr::Header{{"Content-Type", "application/json"},
                                                        {"X-API-Key", API_CLIENT_SECRET}},
                                            cpr::Body{requestBody.dump()},
                                            cpr::Timeout{10000},
                                            cpr::ConnectTimeout{5000});

                if (r.error.code != cpr::ErrorCode::OK || r.status_code != 200) {
                    apiResult.success = false;
                    apiResult.errorMessage = (r.status_code >= 500) ? "api.error.server_unavailable" : "api.error.generic";
                    promise->set_value(apiResult);
                    return;
                }

                json responseBody = json::parse(r.text);

                if (responseBody.value("status", "") != "success") {
                    logger->Warn("Update API application error: {}", responseBody.value("message", "Unknown error"));
                    apiResult.success = false;
                    apiResult.errorMessage = "api.error.generic";
                    promise->set_value(apiResult);
                    return;
                }

                UpdateInfo info;
                info.status = responseBody.value("message", "up_to_date"); 
                std::transform(info.status.begin(), info.status.end(), info.status.begin(), ::tolower);

                info.updateAvailable = responseBody.value("update_available", false);
                info.latestVersion.major = responseBody.value("latest_major", 0);
                info.latestVersion.minor = responseBody.value("latest_minor", 0);
                info.latestVersion.patch = responseBody.value("latest_patch", 0);
                info.formattedLatestVersion = fmt::format("{}.{}.{}", info.latestVersion.major, info.latestVersion.minor, info.latestVersion.patch);

                if (info.updateAvailable) {
                    // Calculate severity locally based on version difference
                    if (info.latestVersion.major > major) {
                        info.severity = "major";
                    } else if (info.latestVersion.minor > minor) {
                        info.severity = "minor";
                    } else {
                        info.severity = "patch";
                    }
                    
                    info.downloadUrl = responseBody.value("download_url", "");
                    info.changelog = responseBody.value("changelog", "No description provided.");
                    
                    logger->Debug("Update check: current={}.{}.{}, latest={}.{}.{}, calculated severity={}", 
                        major, minor, patch, info.latestVersion.major, info.latestVersion.minor, info.latestVersion.patch, info.severity);
                }
                
                apiResult.success = true;
                apiResult.data = info;

            } catch (const json::parse_error& e) {
                logger->Error("JSON parse error (Update): {}", e.what());
                apiResult.success = false;
                apiResult.errorMessage = "api.error.invalid_response";
            } catch (const std::exception& e) {
                logger->Error("Unexpected error (Update): {}", e.what());
                apiResult.success = false;
                apiResult.errorMessage = "api.error.generic";
            }

            promise->set_value(apiResult);
        }).detach();

        return future;
    }


    std::future<ApiResult<std::vector<Patron>>> ApiService::FetchPatronsAsync(const std::string& baseUrl) {
        auto promise = std::make_shared<std::promise<ApiResult<std::vector<Patron>>>>();
        std::future<ApiResult<std::vector<Patron>>> future = promise->get_future();

        std::thread([this, promise, baseUrl]() {
            auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ApiService");
            ApiResult<std::vector<Patron>> apiResult;

            try {
                // 1. Ensure connectivity before proceeding
                if (!EnsureConnectivity(baseUrl)) {
                    std::lock_guard<std::mutex> lock(m_stateMutex);
                    apiResult.success = false;
                    apiResult.errorMessage = m_state.lastErrorMessage;
                    promise->set_value(apiResult);
                    return;
                }

                logger->Debug("Fetching Patrons info...");

                cpr::Response r = cpr::Post(cpr::Url{baseUrl + API_PATRONS_PATH},
                                            cpr::Header{{"X-API-Key", API_CLIENT_SECRET}},
                                            cpr::Timeout{10000},
                                            cpr::ConnectTimeout{5000});

                if (r.error.code != cpr::ErrorCode::OK || r.status_code != 200) {
                    apiResult.success = false;
                    apiResult.errorMessage = "api.error.patrons_fetch_failed";
                    promise->set_value(apiResult);
                    return;
                }

                json responseBody = json::parse(r.text);

                if (!responseBody.is_object() || !responseBody.contains("patrons") || !responseBody["patrons"].is_array()) {
                     throw std::runtime_error("Invalid JSON structure for patrons response.");
                }

                std::vector<Patron> patrons;
                for (const auto& item : responseBody["patrons"]) {
                    Patron p;
                    p.name = item.value("name", "Unknown Patron");
                    p.tier = item.value("tier", 0);
                    patrons.push_back(p);
                }
                
                apiResult.success = true;
                apiResult.data = patrons;

            } catch (const json::parse_error& e) {
                logger->Error("JSON parse error (Patrons): {}", e.what());
                apiResult.success = false;
                apiResult.errorMessage = "api.error.invalid_response";
            } catch (const std::exception& e) {
                logger->Error("Unexpected error (Patrons): {}", e.what());
                apiResult.success = false;
                apiResult.errorMessage = "api.error.generic";
            }

            promise->set_value(apiResult);
        }).detach();

        return future;
    }

    std::future<void> ApiService::TrackUsageAsync(const std::string& baseUrl, std::string uuid, std::string sessionId, std::string buildHash, std::string version, std::string game, std::string gameVersion, std::map<std::string, bool> plugins, std::vector<LogReportEntry> logs) {
        // We use a dummy promise just to return a future as required by interface
        auto promise = std::make_shared<std::promise<void>>();
        std::future<void> future = promise->get_future();

        std::thread([this, promise, baseUrl, uuid, sessionId, buildHash, version, game, gameVersion, plugins, logs]() {
            auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ApiService");
            
            if (!EnsureConnectivity(baseUrl)) {
                promise->set_value();
                return;
            }

            try {
                json logsArray = json::array();
                for (const auto& log : logs) {
                    logsArray.push_back({
                        {"logger", log.loggerName},
                        {"level", log.level},
                        {"message", log.message},
                        {"count", log.count}
                    });
                }

                json requestBody = {
                    {"user_uuid", uuid},
                    {"build_hash", buildHash},                   
                    {"session_id", sessionId},
                    {"version", version},
                    {"game", game},
                    {"game_version", gameVersion},
                    {"plugins", plugins},
                    {"logs", logsArray}
                };

                cpr::Response r = cpr::Post(cpr::Url{baseUrl + API_TRACK_USAGE_PATH},
                                            cpr::Header{{"Content-Type", "application/json"},
                                                        {"X-API-Key", API_CLIENT_SECRET}},
                                            cpr::Body{requestBody.dump()},
                                            cpr::Timeout{10000},
                                            cpr::ConnectTimeout{5000});

                if (r.error.code == cpr::ErrorCode::OK && r.status_code == 200) {
                    if (logs.empty()) {
                        logger->Info("Analytics session processed successfully.");
                    } else {
                        logger->Info("Analytics/Logs session processed successfully. [Logs: {}]", logs.size());
                    }
                } else {
                    logger->Warn("Analytics session failed: HTTP {} [Error: {}]", r.status_code, r.error.message);
                }

            } catch (const std::exception& e) {
                logger->Error("Error in TrackUsageAsync: {}", e.what());
            }
            promise->set_value();
        }).detach();

        return future;
    }

} // namespace System
SPF_NS_END
