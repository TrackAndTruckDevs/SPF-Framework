#include "SPF/Modules/UpdateManager.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Events/SystemEvents.hpp" // For OnUpdateCheckCompleted
#include <algorithm> // Required for std::transform
#include <cctype>    // Required for std::tolower

SPF_NS_BEGIN
namespace Modules {

    UpdateManager::UpdateManager(Events::EventManager& eventManager, System::ApiService& apiService, Config::IConfigService& configService)
        : m_eventManager(eventManager),
          m_apiService(apiService),
          m_configService(configService),
          m_currentUpdateStatus(UpdateStatus::Unknown)
    {
        m_onRequestTrackUsageSink = std::make_unique<Utils::Sink<void(const Events::System::OnRequestTrackUsage&)>>(m_eventManager.System.OnRequestTrackUsage);
    }

    Core::InitializationReport UpdateManager::Initialize() {
        auto logger = Logging::LoggerFactory::GetInstance().GetLogger("UpdateManager");
        logger->Info("Initializing UpdateManager...");

        m_onRequestTrackUsageSink->Connect<&UpdateManager::OnRequestTrackUsage>(this);

        Core::InitializationReport report;
        report.ServiceName = "UpdateManager";
        report.InfoMessages.push_back("UpdateManager initialized successfully.");
        return report;
    }

    void UpdateManager::Shutdown() {
        auto logger = Logging::LoggerFactory::GetInstance().GetLogger("UpdateManager");
        logger->Info("Shutting down UpdateManager...");
        // No sinks to disconnect.
    }

    void UpdateManager::Update() {
        // Check for UpdateInfo future completion
        if (m_updateInfoFuture && m_updateInfoFuture->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            auto logger = Logging::LoggerFactory::GetInstance().GetLogger("UpdateManager");
            logger->Info("Update info fetch completed.");
            System::ApiResult<System::UpdateInfo> result = m_updateInfoFuture->get();
            m_updateInfoFuture.reset(); // Clear the future

            if (result.success && result.data.has_value()) {
                m_lastUpdateInfo = result.data.value();
                // The new API response directly tells us the status, so no local comparison is needed.
                // We just forward the result to the UI.
                logger->Info("Update check successful. Update available: {}", m_lastUpdateInfo->updateAvailable);

                // Dispatch success event
                m_eventManager.System.OnUpdateCheckSucceeded.Call({m_lastUpdateInfo.value()});
            } else {
                logger->Warn("Update info fetch failed: {}", result.errorMessage.value_or("Unknown error"));
                m_lastUpdateInfo.reset();

                // Dispatch failure event
                m_eventManager.System.OnUpdateCheckFailed.Call({result.errorMessage});
            }
        }

        // Check for Patrons future completion
        if (m_patronsFuture && m_patronsFuture->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            auto logger = Logging::LoggerFactory::GetInstance().GetLogger("UpdateManager");
            logger->Info("Patrons fetch completed.");
            System::ApiResult<std::vector<System::Patron>> result = m_patronsFuture->get();
            m_patronsFuture.reset(); // Clear the future

            if (result.success && result.data.has_value()) {
                m_lastPatrons = result.data.value();
                logger->Info("Successfully fetched {} patrons.", m_lastPatrons->size());
            } else {
                logger->Warn("Patrons fetch failed: {}", result.errorMessage.value_or("Unknown error"));
                m_lastPatrons.reset();
            }

            // Dispatch event for UI
            m_eventManager.System.OnPatronsFetchCompleted.Call({result});
        }
    }

    void UpdateManager::RequestUpdateCheck() {
        // Prevent starting a new check if one is already in progress
        if (m_updateInfoFuture.has_value()) {
            return;
        }
        auto logger = Logging::LoggerFactory::GetInstance().GetLogger("UpdateManager");
        logger->Info("Received request to check for updates. Retrieving framework info...");

        const auto& allComponents = m_configService.GetAllComponentInfo();
        auto frameworkIt = allComponents.find("framework");
        if (frameworkIt == allComponents.end()) {
            logger->Error("Framework ComponentInfo not found. Cannot check for updates.");
            std::optional<std::string> error = "api.error.framework_info_missing";
            m_eventManager.System.OnUpdateCheckFailed.Call({error});
            return;
        }
        const auto& frameworkInfo = frameworkIt->second;

        // Check for version
        if (!frameworkInfo.version.has_value() || frameworkInfo.version->empty()) {
            logger->Warn("Framework version missing in ComponentInfo. Cannot check for updates.");
            std::optional<std::string> error = "api.error.framework_version_missing";
            m_eventManager.System.OnUpdateCheckFailed.Call({error});
            return;
        }
        const std::string currentVersion = frameworkInfo.version.value();

        // Check for update URL
        if (!frameworkInfo.websiteUrl.has_value() || frameworkInfo.websiteUrl->empty()) {
            logger->Warn("Framework update URL missing in ComponentInfo. Cannot check for updates.");
            std::optional<std::string> error = "api.error.framework_update_url_missing";
            m_eventManager.System.OnUpdateCheckFailed.Call({error});
            return;
        }
        const std::string updateBaseUrl = frameworkInfo.websiteUrl.value();


        // Determine channel from version string (case-insensitive)
        std::string lowerCaseVersion = currentVersion;
        std::transform(lowerCaseVersion.begin(), lowerCaseVersion.end(), lowerCaseVersion.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        std::string channel = (lowerCaseVersion.find("beta") != std::string::npos) ? "beta" : "release";

        // Parse the current version into major, minor, patch components
        std::optional<System::Version> clientVersionOpt = System::Version::FromString(currentVersion);
        if (!clientVersionOpt.has_value()) {
            logger->Error("Failed to parse client version string '{}'. Cannot check for updates.", currentVersion);
            // Optionally, dispatch an error event or return an ApiResult with error
            std::optional<std::string> error = "api.error.client_version_parse_failed";
            m_eventManager.System.OnUpdateCheckFailed.Call({error});
            return;
        }
        System::Version clientVersionParsed = clientVersionOpt.value();

        logger->Info("Parsed as: Major={}, Minor={}, Patch={}, Channel='{}'.", clientVersionParsed.major, clientVersionParsed.minor, clientVersionParsed.patch, channel);

        m_updateInfoFuture = m_apiService.FetchUpdateInfoAsync(updateBaseUrl, clientVersionParsed.major, clientVersionParsed.minor, clientVersionParsed.patch, channel);
    }

    void UpdateManager::RequestPatronsFetch() {
        // Prevent starting a new check if one is already in progress
        if (m_patronsFuture.has_value()) {
            return;
        }
        auto logger = Logging::LoggerFactory::GetInstance().GetLogger("UpdateManager");
        logger->Info("Received request to fetch patrons. Retrieving framework info...");

        const auto& allComponents = m_configService.GetAllComponentInfo();
        auto frameworkIt = allComponents.find("framework");
        if (frameworkIt == allComponents.end()) {
            logger->Error("Framework ComponentInfo not found. Cannot fetch patrons.");
            m_eventManager.System.OnPatronsFetchCompleted.Call({System::ApiResult<std::vector<System::Patron>>{false, std::nullopt, "api.error.framework_info_missing"}});
            return;
        }
        const auto& frameworkInfo = frameworkIt->second;

        // Check for update URL
        if (!frameworkInfo.websiteUrl.has_value() || frameworkInfo.websiteUrl->empty()) {
            logger->Warn("Framework website URL missing in ComponentInfo. Cannot fetch patrons.");
            m_eventManager.System.OnPatronsFetchCompleted.Call({System::ApiResult<std::vector<System::Patron>>{false, std::nullopt, "api.error.framework_website_url_missing"}});
            return;
        }
        const std::string updateBaseUrl = frameworkInfo.websiteUrl.value();

        logger->Info("Initiating API call to fetch patrons from base URL '{}'.", updateBaseUrl);
        m_patronsFuture = m_apiService.FetchPatronsAsync(updateBaseUrl);
    }

    void UpdateManager::OnRequestTrackUsage(const Events::System::OnRequestTrackUsage& e) {
        auto logger = Logging::LoggerFactory::GetInstance().GetLogger("UpdateManager");
        logger->Info("Received request to track usage. Retrieving required info...");

        const auto& allComponents = m_configService.GetAllComponentInfo();
        auto frameworkIt = allComponents.find("framework");
        if (frameworkIt == allComponents.end()) {
            logger->Error("Framework ComponentInfo not found. Cannot track usage.");
            return;
        }
        const auto& frameworkInfo = frameworkIt->second;

        if (!frameworkInfo.version.has_value() || frameworkInfo.version->empty()) {
            logger->Warn("Framework version missing in ComponentInfo. Cannot track usage.");
            return;
        }
        const std::string currentVersion = frameworkInfo.version.value();

        if (!frameworkInfo.websiteUrl.has_value() || frameworkInfo.websiteUrl->empty()) {
            logger->Warn("Framework update URL missing in ComponentInfo. Cannot track usage.");
            return;
        }
        const std::string updateBaseUrl = frameworkInfo.websiteUrl.value();
        
        std::string instanceId = m_configService.GetOrCreateFrameworkInstanceId();
        if (instanceId == "generation_failed") {
            logger->Error("Failed to get or create framework instance ID. Cannot track usage.");
            return;
        }

        m_apiService.TrackUsageAsync(updateBaseUrl, instanceId, currentVersion);
    }

} // namespace Modules
SPF_NS_END
