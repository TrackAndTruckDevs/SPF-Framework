#include "SPF/System/ApiService.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <cpr/cpr.h> // Include cpr
#include <nlohmann/json.hpp>
#include <fmt/format.h>

#include <chrono>
#include <thread>
#include <random> // For mock error simulation

// Use the nlohmann::json library
using json = nlohmann::json;

namespace {
    // --- Constants for the API ---
    constexpr const char* API_UPDATE_PATH = "/api/v1/check_update.php";
    constexpr const char* API_PATRONS_PATH = "/api/v1/get_patrons.php";
    constexpr const char* API_TRACK_USAGE_PATH = "/api/v1/track_usage.php";
    
    constexpr const char* API_CLIENT_SECRET = "SPF_API_SEC_6ccfd2c1-7b9d-48e1-9f0a-3d2e1c0b9a8f_TRUCKSIMHUB";
}

SPF_NS_BEGIN
namespace System {

    // --- Version Implementation ---
    bool Version::operator>(const Version& other) const {
        if (major != other.major) return major > other.major;
        if (minor != other.minor) return minor > other.minor;
        return patch > other.patch;
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
        return std::async(std::launch::async, [baseUrl, major, minor, patch, channel]() {
            auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ApiService");
            logger->Info("Fetching update info for Major={}, Minor={}, Patch={} on channel '{}' from base URL '{}'...", major, minor, patch, channel, baseUrl);

            ApiResult<UpdateInfo> apiResult;
            std::string response_body_for_logging; // To hold the body for logging in catch block

            try {
                // 1. Construct the request body
                json requestBody = {
                    {"major", major},
                    {"minor", minor},
                    {"patch", patch},
                    {"channel", channel}
                };

                // 2. Perform the POST request using cpr
                cpr::Response r = cpr::Post(cpr::Url{baseUrl + API_UPDATE_PATH},
                                            cpr::Header{{"Content-Type", "application/json"},
                                                        {"X-API-Key", API_CLIENT_SECRET}},
                                            cpr::Body{requestBody.dump()},
                                            cpr::Timeout{10000}, // 10 seconds timeout for the whole request
                                            cpr::ConnectTimeout{5000}); // 5 seconds for connection

                // 3. Handle the response
                response_body_for_logging = r.text;

                if (r.error.code != cpr::ErrorCode::OK) {
                    logger->Error("Update check failed. Connection/request error: [Code {}] {}", static_cast<int>(r.error.code), r.error.message);
                    apiResult.success = false;
                    apiResult.errorMessage = "api.error.no_internet"; // Generic connection error
                    return apiResult;
                }

                if (r.status_code != 200) {
                    logger->Error("Update check failed. HTTP Status: {}. Body: {}", r.status_code, r.text);
                    if (r.status_code >= 500) {
                        apiResult.errorMessage = "api.error.server_unavailable";
                    } else if (r.status_code == 403) {
                         apiResult.errorMessage = "api.error.forbidden"; // API Key issue
                    } else if (r.status_code == 404) {
                         apiResult.errorMessage = "api.error.not_found"; // No release found for channel
                    }
                     else {
                        apiResult.errorMessage = "api.error.generic";
                    }
                    apiResult.success = false;
                    return apiResult;
                }

                // 4. Parse the JSON response
                json responseBody = json::parse(r.text);

                UpdateInfo info;
                info.status = responseBody.value("status", "up_to_date"); // Extract status
                info.updateAvailable = responseBody.value("update_available", false);
                
                // Always try to get the latest version string, even if no update is available.
                    // Populate version components directly
                    info.latestVersion.major = responseBody.value("latest_major", 0);
                    info.latestVersion.minor = responseBody.value("latest_minor", 0);
                    info.latestVersion.patch = responseBody.value("latest_patch", 0);
                    info.formattedLatestVersion = fmt::format("{}.{}.{}", info.latestVersion.major, info.latestVersion.minor, info.latestVersion.patch);


                if (info.updateAvailable) {
                    // If an update is available, all fields should be present
                    info.severity = responseBody.value("severity", "patch");
                    info.downloadUrl = responseBody.value("download_url", "");
                    info.changelog = responseBody.value("changelog", "No description provided.");
                    
                }
                
                apiResult.success = true;
                apiResult.data = info;

            } catch (const json::parse_error& e) {
                logger->Error("Failed to parse JSON response from update server. Error: {}", e.what());
                logger->Error("Raw response body: {}", response_body_for_logging);
                apiResult.success = false;
                apiResult.errorMessage = "api.error.invalid_response";
            } catch (const std::exception& e) {
                logger->Error("An unexpected error occurred during update check: {}", e.what());
                apiResult.success = false;
                apiResult.errorMessage = "api.error.generic";
            }

            return apiResult;
        });
    }


    std::future<ApiResult<std::vector<Patron>>> ApiService::FetchPatronsAsync(const std::string& baseUrl) {
        return std::async(std::launch::async, [baseUrl]() {
            auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ApiService");
            logger->Info("Fetching patrons info...");

            ApiResult<std::vector<Patron>> apiResult;
            std::string response_body_for_logging;

            try {
                cpr::Response r = cpr::Post(cpr::Url{baseUrl + API_PATRONS_PATH},
                                            cpr::Header{{"X-API-Key", API_CLIENT_SECRET}},
                                            cpr::Timeout{10000},
                                            cpr::ConnectTimeout{5000});

                response_body_for_logging = r.text;

                if (r.error.code != cpr::ErrorCode::OK) {
                    logger->Error("Patrons fetch failed. Connection/request error: [Code {}] {}", static_cast<int>(r.error.code), r.error.message);
                    apiResult.success = false;
                    apiResult.errorMessage = "api.error.patrons_fetch_failed";
                    return apiResult;
                }

                if (r.status_code != 200) {
                    logger->Error("Patrons fetch failed. HTTP Status: {}. Body: {}", r.status_code, r.text);
                    apiResult.success = false;
                    apiResult.errorMessage = "api.error.patrons_fetch_failed";
                    return apiResult;
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
                logger->Error("Failed to parse JSON response from patrons API. Error: {}", e.what());
                logger->Error("Raw response body: {}", response_body_for_logging);
                apiResult.success = false;
                apiResult.errorMessage = "api.error.invalid_response";
            } catch (const std::exception& e) {
                logger->Error("An unexpected error occurred during patrons fetch: {}", e.what());
                apiResult.success = false;
                apiResult.errorMessage = "api.error.generic";
            }

            return apiResult;
        });
    }

    std::future<void> ApiService::TrackUsageAsync(const std::string& baseUrl, std::string uuid, std::string version) {
        return std::async(std::launch::async, [baseUrl, uuid, version]() {
            auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ApiService");
            
            try {
                json requestBody = {
                    {"user_uuid", uuid},
                    {"version", version}
                };

                cpr::Response r = cpr::Post(cpr::Url{baseUrl + API_TRACK_USAGE_PATH},
                                            cpr::Header{{"Content-Type", "application/json"},
                                                        {"X-API-Key", API_CLIENT_SECRET}},
                                            cpr::Body{requestBody.dump()},
                                            cpr::Timeout{10000},
                                            cpr::ConnectTimeout{5000});

                if (r.error.code != cpr::ErrorCode::OK) {
                    logger->Debug("Usage tracking failed. Connection/request error: [Code {}] {}", static_cast<int>(r.error.code), r.error.message);
                } else if (r.status_code != 200) {
                    logger->Debug("Usage tracking failed. HTTP Status: {}. Body: {}", r.status_code, r.text);
                } else {
                    logger->Debug("Usage tracking data sent successfully.");
                }
            } catch (const std::exception& e) {
                logger->Debug("An unexpected error occurred during usage tracking: {}", e.what());
            }
            // No return value, fire-and-forget.
        });
    }

} // namespace System
SPF_NS_END
