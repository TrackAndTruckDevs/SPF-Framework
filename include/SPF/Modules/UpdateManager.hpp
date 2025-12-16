#pragma once

#include "SPF/Namespace.hpp"
#include "SPF/Events/EventManager.hpp"        // Required for event subscription/dispatch
#include "SPF/System/ApiService.hpp"          // Required for ApiService and data types
#include "SPF/Core/InitializationReport.hpp"  // For initialization reporting
#include "SPF/Config/IConfigurable.hpp"       //  For IConfigurable base class

#include "SPF/Config/IConfigService.hpp"       // Required for IConfigService

SPF_NS_BEGIN
namespace Modules {

/**
 * @brief Manages the framework's update checking logic and state.
 *
 * This class coordinates with the ApiService to fetch update information
 * and dispatches events based on the update status.
 */
class UpdateManager : public Config::IConfigurable {  //  Inherit from IConfigurable
 public:
  enum class UpdateStatus {
    Unknown,
    UpToDate,
    PatchAvailable,  // e.g., 1.0.0 -> 1.0.1
    MinorAvailable,  // e.g., 1.0.0 -> 1.1.0
    MajorAvailable   // e.g., 1.0.0 -> 2.0.0
  };

  UpdateManager(Events::EventManager& eventManager, System::ApiService& apiService, Config::IConfigService& configService);
  ~UpdateManager() = default;

  Core::InitializationReport Initialize();
  void Shutdown();
  void Update();  // Called per frame to check for async results

  /**
   * @brief Initiates an asynchronous check for framework updates.
   */
  void RequestUpdateCheck();

  /**
   * @brief Initiates an asynchronous fetch for the patrons list.
   */
  void RequestPatronsFetch();

  /**
   * @brief Retrieves the current update status of the framework.
   * @return The current UpdateStatus enum value.
   */
  UpdateStatus GetCurrentUpdateStatus() const { return m_currentUpdateStatus; }

  /**
   * @brief Retrieves the last fetched update information.
   * @return An optional containing UpdateInfo if available.
   */
  std::optional<System::UpdateInfo> GetLastUpdateInfo() const { return m_lastUpdateInfo; }

  /**
   * @brief Retrieves the last fetched patrons list.
   * @return An optional containing the vector of Patrons if available.
   */
  std::optional<std::vector<System::Patron>> GetLastPatrons() const { return m_lastPatrons; }

  // --- IConfigurable Implementation ---
  bool OnSettingChanged(const std::string& systemName, const std::string& componentName, const std::string& keyPath, const nlohmann::json& newValue) override {
    return false;  // UpdateManager does not have its own configurable settings for now
  }

 private:
  // Event handler
  void OnRequestTrackUsage(const Events::System::OnRequestTrackUsage& e);

  Events::EventManager& m_eventManager;
  System::ApiService& m_apiService;
  Config::IConfigService& m_configService;
  UpdateStatus m_currentUpdateStatus = UpdateStatus::Unknown;

  // Futures to hold the results of async API calls
  std::optional<std::future<System::ApiResult<System::UpdateInfo>>> m_updateInfoFuture;
  std::optional<std::future<System::ApiResult<std::vector<System::Patron>>>> m_patronsFuture;

  std::optional<System::UpdateInfo> m_lastUpdateInfo;
  std::optional<std::vector<System::Patron>> m_lastPatrons;

  // Event Sinks
  std::unique_ptr<Utils::Sink<void(const Events::System::OnRequestTrackUsage&)>> m_onRequestTrackUsageSink;

  // Utility to compare versions and determine UpdateStatus
  UpdateStatus CompareVersions(const System::Version& current, const System::Version& latest);
};

}  // namespace Modules
SPF_NS_END
