#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/Config/IConfigService.hpp"
#include "SPF/Core/InitializationReport.hpp"
#include "SPF/Events/EventManager.hpp"
#include "SPF/Events/SystemEvents.hpp"
#include "SPF/Logging/Sinks/ErrorReportSink.hpp"
#include "SPF/System/ApiService.hpp"
#include "SPF/Utils/Signal.hpp"

#include <chrono>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>


SPF_NS_BEGIN
namespace Modules {

/**
 * @brief Manages all communications with the remote API, providing caching and smart request handling.
 */
class CommunicationManager {
 public:
  enum class UpdateStatus { Unknown, UpToDate, PatchAvailable, MinorAvailable, MajorAvailable };

  enum class ResourceStatus { NotLoaded, Loading, Ready, Error, Banned };

  template <typename T>
  struct ResourceState {
    ResourceStatus status = ResourceStatus::NotLoaded;
    std::optional<T> data;
    std::optional<std::string> lastErrorMessage;
    std::chrono::steady_clock::time_point lastErrorTime;
  };

  CommunicationManager(Events::EventManager& eventManager, System::ApiService& apiService, Config::IConfigService& configService);
  ~CommunicationManager() = default;

  // --- Lifecycle ---
  Core::InitializationReport Initialize();
  void Shutdown();
  void Update();

  // --- Resource Management (GET) ---
  /**
   * @brief Requests an update check. Returns cached data unless forceRefresh is true or timer expired.
   */
  void RequestUpdateCheck(bool forceRefresh = false);

  /**
   * @brief Requests patrons fetch. Returns cached data unless forceRefresh is true or timer expired.
   */
  void RequestPatronsFetch(bool forceRefresh = false);

  /**
   * @brief Requests an update check for all enabled plugins that have a GitHub URL.
   */
  void RequestPluginUpdateChecks();

  /**
   * @brief Requests release notes for the current framework version.
   */
  void RequestReleaseNotesFetch();

  // --- Data Submission (SET) ---
  /**
   * @brief Queues analytics session data for submission.
   */
  void RequestTrackUsage();

  // --- Signals ---
  Utils::Signal<void(const System::UpdateInfo&)> OnUpdateInfoReceived;
  Utils::Signal<void(const System::ChangelogData&)> OnReleaseNotesReceived;
  Utils::Signal<void(const Events::System::OnPluginUpdateAvailable&)> OnPluginUpdateAvailable;

 private:
  // --- Event Handlers ---
  void OnRequestTrackUsage(const Events::System::OnRequestTrackUsage& e);
  void OnErrorReportSinkChanged(std::shared_ptr<Logging::Sinks::ErrorReportSink> newSink);

  // --- Internal Helpers ---
  /**
   * @brief Ensures connection permission is loaded from config.
   */
  void EnsurePermission();

  bool ShouldPerformRequest(ResourceStatus status, std::chrono::steady_clock::time_point lastErrorTime, bool forceRefresh);

  struct GithubRepo {
    std::string owner;
    std::string repo;
  };
  std::optional<GithubRepo> ParseGithubUrl(const std::string& url);

  Events::EventManager& m_eventManager;
  System::ApiService& m_apiService;
  Config::IConfigService& m_configService;

  bool m_connectionAllowed = true;   // Cached permission
  bool m_permissionChecked = false;  // Flag to ensure single load from config

  // --- Session Management ---
  std::string m_sessionId;
  std::shared_ptr<Logging::Sinks::ErrorReportSink> m_errorSink;
  std::chrono::steady_clock::time_point m_lastUsageTrackTime;
  std::map<std::string, bool> m_lastSentPlugins;
  bool m_hasInitialTrackingSent = false;

  ResourceState<System::UpdateInfo> m_updateState;
  ResourceState<std::vector<System::Patron>> m_patronsState;
  ResourceState<System::ChangelogData> m_releaseNotesState;

  // Key: pluginId, Value: Future for GitHub update check
  std::map<std::string, std::future<System::ApiResult<System::GithubReleaseInfo>>> m_pluginUpdateFutures;
  std::map<std::string, std::chrono::steady_clock::time_point> m_lastPluginCheckTimes;
  std::mutex m_stateMutex;

  // --- Futures for async processing ---
  std::optional<std::future<System::ApiResult<System::UpdateInfo>>> m_updateFuture;
  std::optional<std::future<System::ApiResult<std::vector<System::Patron>>>> m_patronsFuture;
  std::optional<std::future<System::ApiResult<System::ChangelogData>>> m_releaseNotesFuture;
  std::optional<std::future<void>> m_trackUsageFuture;

  // --- Sinks ---
  std::unique_ptr<Utils::Sink<void(const Events::System::OnRequestTrackUsage&)>> m_onRequestTrackUsageSink;
  std::unique_ptr<Utils::Sink<void(std::shared_ptr<Logging::Sinks::ErrorReportSink>)>> m_onErrorReportSinkChangedSink;
};

}  // namespace Modules
SPF_NS_END
