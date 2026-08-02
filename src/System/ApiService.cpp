#include "SPF/System/ApiService.hpp"

#include "SPF/Namespace.hpp"

#include "SPF/Logging/LoggerFactory.hpp"

#include "cpr/api.h"
#include "cpr/body.h"
#include "cpr/connect_timeout.h"
#include "cpr/cprtypes.h"
#include "cpr/error.h"
#include "cpr/response.h"
#include "cpr/timeout.h"
#include "nlohmann/json.hpp"  // IWYU pragma: keep
#include "nlohmann/json_fwd.hpp"

#include <chrono>
#include <cpr/cpr.h>  // Include cpr
#include <cstddef>
#include <exception>
#include <fmt/format.h>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

// IWYU insists on a direct provider for _s functions.
// MinGW: pull in MSVC-compat decl; MSVC gets them from <cstdio> natively.
#if defined(__MINGW32__) || defined(__MINGW64__)
#include <sec_api/stdio_s.h>
#endif

// Use the nlohmann::ordered_json library
using json = nlohmann::ordered_json;

namespace {
// --- Constants for the API ---
constexpr const char* API_HEALTH_PATH = "/api/v1/health.php";
constexpr const char* API_UPDATE_PATH = "/api/v1/get_framework_update.php";
constexpr const char* API_NOTES_PATH = "/api/v1/get_release_notes.php";
constexpr const char* API_PATRONS_PATH = "/api/v1/get_patrons.php";
constexpr const char* API_TRACK_USAGE_PATH = "/api/v1/track_usage.php";

constexpr const char* API_CLIENT_SECRET = "SPF_API_SEC_6ccfd2c1-7b9d-48e1-9f0a-3d2e1c0b9a8f_TRUCKSIMHUB";

constexpr auto HEALTH_CHECK_INTERVAL = std::chrono::minutes(5);
}  // namespace

SPF_NS_BEGIN
namespace System {

// --- ApiService Implementation ---

ApiService::ApiService() { m_state.lastCheckTime = std::chrono::steady_clock::now() - HEALTH_CHECK_INTERVAL - std::chrono::seconds(1); }

ApiService::~ApiService() { Shutdown(); }

void ApiService::WorkerLoop() {
  while (true) {
    std::function<void()> task;
    {
      std::unique_lock<std::mutex> lock(m_taskMutex);
      m_taskCV.wait(lock, [this] { return m_shutdown || !m_tasks.empty(); });
      if (m_shutdown) break;
      task = std::move(m_tasks.front());
      m_tasks.pop();
    }
    task();
  }
}

void ApiService::EnsureWorkerStarted() {
  std::lock_guard<std::mutex> lock(m_taskMutex);
  if (m_workerThread.joinable()) return;
  m_workerThread = std::thread(&ApiService::WorkerLoop, this);
}

void ApiService::PostTask(std::function<void()> task) {
  EnsureWorkerStarted();
  {
    std::lock_guard<std::mutex> lock(m_taskMutex);
    m_tasks.push(std::move(task));
  }
  m_taskCV.notify_one();
}

void ApiService::Shutdown() {
  {
    std::lock_guard<std::mutex> lock(m_taskMutex);
    m_shutdown = true;
  }
  m_taskCV.notify_all();
  if (m_workerThread.joinable()) {
    m_workerThread.join();
  }
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
  lock.unlock();  // Unlock while performing network I/O

  auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ApiService");

  try {
    cpr::Response r = cpr::Post(cpr::Url{baseUrl + API_HEALTH_PATH}, cpr::Header{{"X-API-Key", API_CLIENT_SECRET}}, cpr::Timeout{5000}, cpr::ConnectTimeout{3000});

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

    lock.lock();  // Re-lock to update state

    if (newStatus != m_state.status) {
      if (newStatus == ServiceStatus::Online) {
        logger->Info("API Service is Online. [BaseUrl: {}]", baseUrl);
      } else {
        logger->Warn("API Service state changed: {} [HTTP: {}, Error: {}]", (newStatus == ServiceStatus::Banned ? "BANNED" : "OFFLINE/ERROR"), r.status_code, r.error.message);
      }
    }

    m_state.status = newStatus;
    m_state.lastErrorMessage = errorMsg;
    m_state.lastCheckTime = std::chrono::steady_clock::now();
    m_state.isChecking = false;

    lock.unlock();
    m_connectivityCV.notify_all();  // Wake up everyone waiting

    return newStatus == ServiceStatus::Online;

  } catch (const std::exception& e) {
    logger->Error("Unexpected error during health check: {}", e.what());

    lock.lock();
    m_state.isChecking = false;
    m_state.lastCheckTime = std::chrono::steady_clock::now();  // Prevent immediate retry
    lock.unlock();
    m_connectivityCV.notify_all();

    return false;
  }
}

// --- Version Implementation ---
bool Version::operator>(const Version& other) const {
  if (major != other.major) return major > other.major;
  if (minor != other.minor) return minor > other.minor;
  if (patch != other.patch) return patch > other.patch;
  return revision > other.revision;
}

bool Version::operator<(const Version& other) const {
  if (major != other.major) return major < other.major;
  if (minor != other.minor) return minor < other.minor;
  if (patch != other.patch) return patch < other.patch;
  return revision < other.revision;
}

bool Version::operator==(const Version& other) const { return major == other.major && minor == other.minor && patch == other.patch && revision == other.revision; }

std::string Version::ToString() const {
  if (revision > 0) {
    return fmt::format("{}.{}.{}.{}", major, minor, patch, revision);
  }
  return fmt::format("{}.{}.{}", major, minor, patch);
}

// More robust implementation of FromString
std::optional<Version> Version::FromString(const std::string& versionStr) {
  if (versionStr.empty()) return std::nullopt;

  // Find the first digit in the string to skip prefixes like "v", "vers", etc.
  size_t firstDigit = versionStr.find_first_of("0123456789");
  if (firstDigit == std::string::npos) return std::nullopt;

  const char* start = versionStr.c_str() + firstDigit;
  Version v;

  // Attempt to parse up to 4 components: major.minor.patch.revision
  if (sscanf_s(start, "%d.%d.%d.%d", &v.major, &v.minor, &v.patch, &v.revision) >= 4) {
    return v;
  }
  if (sscanf_s(start, "%d.%d.%d", &v.major, &v.minor, &v.patch) >= 3) {
    v.revision = 0;
    return v;
  }
  if (sscanf_s(start, "%d.%d", &v.major, &v.minor) >= 2) {
    v.patch = 0;
    v.revision = 0;
    return v;
  }
  if (sscanf_s(start, "%d", &v.major) >= 1) {
    v.minor = 0;
    v.patch = 0;
    v.revision = 0;
    return v;
  }

  return std::nullopt;
}

// --- ApiService Implementation ---
std::future<ApiResult<UpdateInfo>> ApiService::FetchUpdateInfoAsync(const std::string& baseUrl, int major, int minor, int patch, const std::string& channel, const std::string& lang) {
  auto promise = std::make_shared<std::promise<ApiResult<UpdateInfo>>>();
  std::future<ApiResult<UpdateInfo>> future = promise->get_future();

  PostTask([this, promise, baseUrl, major, minor, patch, channel, lang]() {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ApiService");
    ApiResult<UpdateInfo> apiResult;

    try {
      if (!EnsureConnectivity(baseUrl)) {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        apiResult.success = false;
        apiResult.errorMessage = m_state.lastErrorMessage;
        promise->set_value(apiResult);
        return;
      }

      logger->Debug("Checking for framework updates (current v{}.{}.{})...", major, minor, patch);

      json requestBody = {{"major", major}, {"minor", minor}, {"patch", patch}, {"channel", channel}, {"lang", lang}};

      cpr::Response r = cpr::Post(cpr::Url{baseUrl + API_UPDATE_PATH}, cpr::Header{{"Content-Type", "application/json"}, {"X-API-Key", API_CLIENT_SECRET}}, cpr::Body{requestBody.dump()}, cpr::Timeout{10000}, cpr::ConnectTimeout{5000});

      if (r.error.code != cpr::ErrorCode::OK || r.status_code != 200) {
        apiResult.success = false;
        apiResult.errorMessage = (r.status_code >= 500) ? "api.error.server_unavailable" : "api.error.generic";
        promise->set_value(apiResult);
        return;
      }

      json responseBody = json::parse(r.text);
      if (responseBody.value("status", "") != "success") {
        apiResult.success = false;
        apiResult.errorMessage = "api.error.generic";
        promise->set_value(apiResult);
        return;
      }

      // Safe extraction of "data" object
      json data = responseBody.value("data", json::object());
      if (!data.is_object()) data = json::object();

      UpdateInfo info;
      info.updateAvailable = data.value("update_available", false);

      if (info.updateAvailable) {
        // Safe extraction of "version"
        json v = data.value("version", json::object());
        info.latestVersion.ver.major = v.value("major", 0);
        info.latestVersion.ver.minor = v.value("minor", 0);
        info.latestVersion.ver.patch = v.value("patch", 0);
        info.latestVersion.full = v.value("full", "unknown");

        info.downloadUrl = data.value("download_url", "");

        // Safe extraction of "md5"
        json m = data.value("md5", json::object());
        info.md5.archive = m.value("archive", "");
        info.md5.binary = m.value("binary", "");

        // Safe extraction of "content"
        json c = data.value("content", json::object());
        info.content.title = c.value("title", "");
        info.content.markdown = c.value("markdown", "");

        logger->Info("Update available: v{} -> v{}", fmt::format("{}.{}.{}", major, minor, patch), info.latestVersion.full);
      }

      apiResult.success = true;
      apiResult.data = info;

    } catch (const std::exception& e) {
      logger->Error("API Update Check Error: {}", e.what());
      apiResult.success = false;
      apiResult.errorMessage = "api.error.generic";
    }

    promise->set_value(apiResult);
  });

  return future;
}

std::future<ApiResult<ChangelogData>> ApiService::FetchReleaseNotesAsync(const std::string& baseUrl, int major, int minor, int patch, const std::string& lang) {
  auto promise = std::make_shared<std::promise<ApiResult<ChangelogData>>>();
  std::future<ApiResult<ChangelogData>> future = promise->get_future();

  PostTask([this, promise, baseUrl, major, minor, patch, lang]() {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ApiService");
    ApiResult<ChangelogData> apiResult;

    try {
      if (!EnsureConnectivity(baseUrl)) {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        apiResult.success = false;
        apiResult.errorMessage = m_state.lastErrorMessage;
        promise->set_value(apiResult);
        return;
      }

      json requestBody = {{"major", major}, {"minor", minor}, {"patch", patch}, {"lang", lang}};

      cpr::Response r = cpr::Post(cpr::Url{baseUrl + API_NOTES_PATH}, cpr::Header{{"Content-Type", "application/json"}, {"X-API-Key", API_CLIENT_SECRET}}, cpr::Body{requestBody.dump()}, cpr::Timeout{10000}, cpr::ConnectTimeout{5000});

      if (r.error.code != cpr::ErrorCode::OK || r.status_code != 200) {
        apiResult.success = false;
        apiResult.errorMessage = "api.error.generic";
        promise->set_value(apiResult);
        return;
      }

      json responseBody = json::parse(r.text);
      if (responseBody.value("status", "") != "success") {
        apiResult.success = false;
        apiResult.errorMessage = "api.error.content_not_found";
        promise->set_value(apiResult);
        return;
      }

      json data = responseBody.value("data", json::object());
      ChangelogData notes;

      json c = data.value("content", json::object());
      notes.title = c.value("title", "");
      notes.markdown = c.value("markdown", "");

      apiResult.success = true;
      apiResult.data = notes;

    } catch (const std::exception& e) {
      logger->Error("API Release Notes Error: {}", e.what());
      apiResult.success = false;
      apiResult.errorMessage = "api.error.generic";
    }

    promise->set_value(apiResult);
  });

  return future;
}

std::future<ApiResult<std::vector<Patron>>> ApiService::FetchPatronsAsync(const std::string& baseUrl) {
  auto promise = std::make_shared<std::promise<ApiResult<std::vector<Patron>>>>();
  std::future<ApiResult<std::vector<Patron>>> future = promise->get_future();

  PostTask([this, promise, baseUrl]() {
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

      cpr::Response r = cpr::Post(cpr::Url{baseUrl + API_PATRONS_PATH}, cpr::Header{{"X-API-Key", API_CLIENT_SECRET}}, cpr::Timeout{10000}, cpr::ConnectTimeout{5000});

      if (r.error.code != cpr::ErrorCode::OK || r.status_code != 200) {
        apiResult.success = false;
        apiResult.errorMessage = "api.error.patrons_fetch_failed";
        promise->set_value(apiResult);
        return;
      }

      json responseBody = json::parse(r.text);

      if (!responseBody.is_object() || !responseBody.contains("data")) {
        throw std::runtime_error("Invalid JSON structure: missing 'data' object.");
      }

      json data = responseBody["data"];
      if (!data.contains("patrons") || !data["patrons"].is_array()) {
        throw std::runtime_error("Invalid JSON structure: 'patrons' array not found in 'data'.");
      }

      std::vector<Patron> patrons;
      for (const auto& item : data["patrons"]) {
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
  });

  return future;
}

std::future<void> ApiService::TrackUsageAsync(const std::string& baseUrl, std::string uuid, std::string sessionId, std::string buildHash, std::string version, std::string game, std::string gameVersion, std::map<std::string, bool> plugins,
                                              std::vector<LogReportEntry> logs) {
  // We use a dummy promise just to return a future as required by interface
  auto promise = std::make_shared<std::promise<void>>();
  std::future<void> future = promise->get_future();

  PostTask([this, promise, baseUrl, uuid, sessionId, buildHash, version, game, gameVersion, plugins, logs]() {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ApiService");

    if (!EnsureConnectivity(baseUrl)) {
      promise->set_value();
      return;
    }

    try {
      json logsArray = json::array();
      for (const auto& log : logs) {
        logsArray.push_back({{"logger", log.loggerName}, {"level", log.level}, {"message", log.message}, {"count", log.count}});
      }

      json requestBody = {{"user_uuid", uuid}, {"build_hash", buildHash}, {"session_id", sessionId}, {"version", version}, {"game", game}, {"game_version", gameVersion}, {"plugins", plugins}, {"logs", logsArray}};

      cpr::Response r = cpr::Post(cpr::Url{baseUrl + API_TRACK_USAGE_PATH}, cpr::Header{{"Content-Type", "application/json"}, {"X-API-Key", API_CLIENT_SECRET}}, cpr::Body{requestBody.dump()}, cpr::Timeout{10000}, cpr::ConnectTimeout{5000});

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
  });

  return future;
}

std::future<ApiResult<GithubReleaseInfo>> ApiService::FetchGithubLatestReleaseAsync(const std::string& owner, const std::string& repo) {
  auto promise = std::make_shared<std::promise<ApiResult<GithubReleaseInfo>>>();
  std::future<ApiResult<GithubReleaseInfo>> future = promise->get_future();

  PostTask([promise, owner, repo]() {
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("ApiService");
    ApiResult<GithubReleaseInfo> apiResult;

    try {
      logger->Debug("Checking GitHub for latest release of {}/{}...", owner, repo);

      // GitHub API for latest tags (more reliable for non-release versions)
      std::string url = fmt::format("https://api.github.com/repos/{}/{}/tags?per_page=1", owner, repo);

      cpr::Response r = cpr::Get(cpr::Url{url}, cpr::Header{{"Accept", "application/vnd.github.v3+json"}, {"User-Agent", "SPF-Framework-Updater"}}, cpr::Timeout{10000}, cpr::ConnectTimeout{5000});

      if (r.error.code != cpr::ErrorCode::OK) {
        logger->Error("GitHub API Network Error: {} (Repo: {}/{})", r.error.message, owner, repo);
        apiResult.success = false;
        apiResult.errorMessage = "api.error.no_internet";
        promise->set_value(apiResult);
        return;
      }

      if (r.status_code == 200) {
        json responseBody = json::parse(r.text);
        GithubReleaseInfo info;

        if (responseBody.is_array() && !responseBody.empty()) {
          const auto& latestTag = responseBody[0];
          info.tagName = latestTag.value("name", "");
          // For tags, we point to the releases/tag page which usually exists or redirects correctly
          info.htmlUrl = fmt::format("https://github.com/{}/{}/releases/tag/{}", owner, repo, info.tagName);
          info.body = "";  // Tags don't have a body like releases do
        }

        apiResult.success = !info.tagName.empty();
        apiResult.data = info;

        std::string remaining = r.header["X-RateLimit-Remaining"];
        logger->Debug("GitHub API Success: Found tag '{}' for {}/{} (Remaining: {})", info.tagName, owner, repo, remaining);
      } else {
        std::string remaining = r.header["X-RateLimit-Remaining"];
        logger->Warn("GitHub API Error: HTTP {} (Repo: {}/{}, Remaining: {})", r.status_code, owner, repo, remaining);
        logger->Debug("GitHub Response Body: {}", r.text);

        if (r.status_code == 404) {
          apiResult.success = false;
          apiResult.errorMessage = "api.error.content_not_found";
        } else if (r.status_code == 403) {
          apiResult.success = false;
          apiResult.errorMessage = "api.error.rate_limit";
        } else {
          apiResult.success = false;
          apiResult.errorMessage = "api.error.generic";
        }
      }

    } catch (const std::exception& e) {
      logger->Error("GitHub API Error: {}", e.what());
      apiResult.success = false;
      apiResult.errorMessage = "api.error.generic";
    }

    promise->set_value(apiResult);
  });

  return future;
}

}  // namespace System
SPF_NS_END
