#include "SPF/Modules/CommunicationManager.hpp"
#include "SPF/Logging/LoggerFactory.hpp"
#include "SPF/Events/SystemEvents.hpp"
#include "SPF/System/EnvironmentManager.hpp"
#include <algorithm>
#include <cctype>
#include <random>

SPF_NS_BEGIN
namespace Modules {

    namespace {
        constexpr auto RETRY_INTERVAL = std::chrono::minutes(5);
        constexpr auto TRACKING_INTERVAL = std::chrono::minutes(5); // Send logs every 5 mins if pending
    }

    CommunicationManager::CommunicationManager(Events::EventManager& eventManager, System::ApiService& apiService, Config::IConfigService& configService)
        : m_eventManager(eventManager),
          m_apiService(apiService),
          m_configService(configService)
    {
        m_onRequestTrackUsageSink = std::make_unique<Utils::Sink<void(const Events::System::OnRequestTrackUsage&)>>(m_eventManager.System.OnRequestTrackUsage);
        m_onErrorReportSinkChangedSink = std::make_unique<Utils::Sink<void(std::shared_ptr<Logging::Sinks::ErrorReportSink>)>>(Logging::LoggerFactory::GetInstance().OnErrorReportSinkChanged);
        
        // Generate a simple unique session ID for this run
        auto now = std::chrono::system_clock::now().time_since_epoch().count();
        std::mt19937 gen(static_cast<unsigned int>(now));
        std::uniform_int_distribution<unsigned int> dis(10000, 99999);
        m_sessionId = std::to_string(now) + "-" + std::to_string(dis(gen));
    }

    Core::InitializationReport CommunicationManager::Initialize() {
        auto logger = Logging::LoggerFactory::GetInstance().GetLogger("CommunicationManager");
        logger->Info("Initializing CommunicationManager...");

        // Initial sink state
        m_errorSink = Logging::LoggerFactory::GetInstance().GetErrorReportSink();

        // Subscribe to changes
        m_onErrorReportSinkChangedSink->Connect<&CommunicationManager::OnErrorReportSinkChanged>(this);

        m_onRequestTrackUsageSink->Connect<&CommunicationManager::OnRequestTrackUsage>(this);

        Core::InitializationReport report;
        report.ServiceName = "CommunicationManager";
        report.InfoMessages.push_back("CommunicationManager initialized successfully.");
        return report;
    }

    void CommunicationManager::Shutdown() {
        auto logger = Logging::LoggerFactory::GetInstance().GetLogger("CommunicationManager");
        logger->Info("Shutting down CommunicationManager...");
    }

    void CommunicationManager::Update() {
        auto logger = Logging::LoggerFactory::GetInstance().GetLogger("CommunicationManager");
        auto now = std::chrono::steady_clock::now();

        // 1. Check UpdateInfo future
        if (m_updateFuture && m_updateFuture->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            System::ApiResult<System::UpdateInfo> result = m_updateFuture->get();
            m_updateFuture.reset();

            std::lock_guard<std::mutex> lock(m_stateMutex);
            if (result.success) {
                m_updateState.status = ResourceStatus::Ready;
                m_updateState.data = result.data;
                logger->Debug("Update info successfully cached.");
            } else {
                if (result.errorMessage.value_or("") == "api.error.forbidden") {
                    m_updateState.status = ResourceStatus::Banned;
                } else {
                    m_updateState.status = ResourceStatus::Error;
                    m_updateState.lastErrorTime = now;
                }
                m_updateState.lastErrorMessage = result.errorMessage;
            }
            m_eventManager.System.OnUpdateCheckCompleted.Call({result});
        }

        // 2. Check Patrons future
        if (m_patronsFuture && m_patronsFuture->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            System::ApiResult<std::vector<System::Patron>> result = m_patronsFuture->get();
            m_patronsFuture.reset();

            std::lock_guard<std::mutex> lock(m_stateMutex);
            if (result.success) {
                m_patronsState.status = ResourceStatus::Ready;
                m_patronsState.data = result.data;
                logger->Debug("Patrons list successfully cached.");
            } else {
                if (result.errorMessage.value_or("") == "api.error.forbidden") {
                    m_patronsState.status = ResourceStatus::Banned;
                } else {
                    m_patronsState.status = ResourceStatus::Error;
                    m_patronsState.lastErrorTime = now;
                }
                m_patronsState.lastErrorMessage = result.errorMessage;
            }
            m_eventManager.System.OnPatronsFetchCompleted.Call({result});
        }

        // 3. Check TrackUsage future
        if (m_trackUsageFuture && m_trackUsageFuture->valid() && m_trackUsageFuture->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            m_trackUsageFuture.reset();
            m_eventManager.System.OnUsageTrackingCompleted.Call({true});
        }

        // 4. Periodic tracking (Logs OR Plugin state changes)
        bool shouldReport = (m_errorSink && m_errorSink->HasPendingLogs());
        
        if (!shouldReport) {
            // Check if plugins changed since last send
            const auto& allComponents = m_configService.GetAllComponentInfo();
            for (const auto& [id, info] : allComponents) {
                if (!info.isFramework) {
                    if (m_lastSentPlugins.count(id) == 0 || m_lastSentPlugins.at(id) != info.isEnabled) {
                        shouldReport = true;
                        break;
                    }
                }
            }
        }

        if (shouldReport && m_hasInitialTrackingSent) {
            if (now - m_lastUsageTrackTime >= TRACKING_INTERVAL) {
                RequestTrackUsage();
            }
        }
    }

    void CommunicationManager::EnsurePermission() {
        if (m_permissionChecked) return;

        m_connectionAllowed = m_configService.IsConnectionAllowed();
        m_permissionChecked = true;

        if (!m_connectionAllowed) {
            auto logger = Logging::LoggerFactory::GetInstance().GetLogger("CommunicationManager");
            logger->Info("Network communications are disabled by user configuration.");
        }
    }

    bool CommunicationManager::ShouldPerformRequest(ResourceStatus status, std::chrono::steady_clock::time_point lastErrorTime, bool forceRefresh) {
        if (status == ResourceStatus::Loading) return false; // Already in progress
        if (status == ResourceStatus::Banned) return false; // Never retry if banned
        if (forceRefresh) return true; // Explicitly requested bypass
        
        if (status == ResourceStatus::Ready) return false; // Already have valid data
        
        if (status == ResourceStatus::Error) {
            auto now = std::chrono::steady_clock::now();
            return (now - lastErrorTime >= RETRY_INTERVAL); // Retry only if interval passed
        }

        return true; // NotLoaded or other cases
    }

    void CommunicationManager::RequestUpdateCheck(bool forceRefresh) {
        EnsurePermission();
        if (!m_connectionAllowed) return;

        std::lock_guard<std::mutex> lock(m_stateMutex);
        if (!ShouldPerformRequest(m_updateState.status, m_updateState.lastErrorTime, forceRefresh)) {
            // If we have cached result (Success, Banned or recent Error), fire it immediately
            if (m_updateState.status != ResourceStatus::NotLoaded && m_updateState.status != ResourceStatus::Loading) {
                System::ApiResult<System::UpdateInfo> cachedResult;
                cachedResult.success = (m_updateState.status == ResourceStatus::Ready);
                cachedResult.data = m_updateState.data;
                cachedResult.errorMessage = m_updateState.lastErrorMessage;
                m_eventManager.System.OnUpdateCheckCompleted.Call({cachedResult});
            }
            return;
        }

        auto logger = Logging::LoggerFactory::GetInstance().GetLogger("CommunicationManager");
        const auto& allComponents = m_configService.GetAllComponentInfo();
        auto it = allComponents.find("framework");
        if (it == allComponents.end()) return;

        const auto& info = it->second;
        if (!info.version || !info.websiteUrl) return;

        std::string currentVersion = *info.version;
        std::string updateBaseUrl = *info.websiteUrl;

        std::string lowerVersion = currentVersion;
        std::transform(lowerVersion.begin(), lowerVersion.end(), lowerVersion.begin(), ::tolower);
        std::string channel = (lowerVersion.find("beta") != std::string::npos) ? "beta" : "release";

        auto versionOpt = System::Version::FromString(currentVersion);
        if (!versionOpt) return;

        m_updateState.status = ResourceStatus::Loading;
        m_updateFuture = m_apiService.FetchUpdateInfoAsync(updateBaseUrl, versionOpt->major, versionOpt->minor, versionOpt->patch, channel);
    }

    void CommunicationManager::RequestPatronsFetch(bool forceRefresh) {
        EnsurePermission();
        if (!m_connectionAllowed) return;

        std::lock_guard<std::mutex> lock(m_stateMutex);
        if (!ShouldPerformRequest(m_patronsState.status, m_patronsState.lastErrorTime, forceRefresh)) {
            if (m_patronsState.status != ResourceStatus::NotLoaded && m_patronsState.status != ResourceStatus::Loading) {
                System::ApiResult<std::vector<System::Patron>> cachedResult;
                cachedResult.success = (m_patronsState.status == ResourceStatus::Ready);
                cachedResult.data = m_patronsState.data;
                cachedResult.errorMessage = m_patronsState.lastErrorMessage;
                m_eventManager.System.OnPatronsFetchCompleted.Call({cachedResult});
            }
            return;
        }

        const auto& allComponents = m_configService.GetAllComponentInfo();
        auto it = allComponents.find("framework");
        if (it == allComponents.end() || !it->second.websiteUrl) return;

        m_patronsState.status = ResourceStatus::Loading;
        m_patronsFuture = m_apiService.FetchPatronsAsync(*it->second.websiteUrl);
    }

    void CommunicationManager::RequestTrackUsage() {
        EnsurePermission();
        if (!m_connectionAllowed) return;

        // 1. If another request is currently in progress, skip this one
        if (m_trackUsageFuture && m_trackUsageFuture->valid() && m_trackUsageFuture->wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;

        const auto& allComponents = m_configService.GetAllComponentInfo();
        auto it = allComponents.find("framework");
        if (it == allComponents.end() || !it->second.version || !it->second.websiteUrl) return;

        // 2. Check if there's actually anything NEW to send (if not the first time)
        bool hasPluginChanges = false;
        std::map<std::string, bool> currentPlugins;
        for (const auto& [id, info] : allComponents) {
            if (!info.isFramework) {
                currentPlugins[id] = info.isEnabled;
                if (m_lastSentPlugins.count(id) == 0 || m_lastSentPlugins.at(id) != info.isEnabled) {
                    hasPluginChanges = true;
                }
            }
        }

        auto errorSink = Logging::LoggerFactory::GetInstance().GetErrorReportSink();
        bool hasLogs = (errorSink && errorSink->HasPendingLogs());

        // Bail out if already sent and nothing changed
        if (m_hasInitialTrackingSent && !hasPluginChanges && !hasLogs) {
            return;
        }

        auto& env = System::EnvironmentManager::GetInstance();
        m_lastSentPlugins = currentPlugins;

        std::vector<System::LogReportEntry> logs;
        if (errorSink) {
            auto pendingLogs = errorSink->GetAndClearPendingLogs();
            for (const auto& log : pendingLogs) {
                System::LogReportEntry apiLog;
                apiLog.loggerName = log.loggerName;
                apiLog.level = Logging::LogLevelToString(log.level);
                apiLog.message = log.message;
                apiLog.count = log.count;
                logs.push_back(apiLog);
            }
        }

        m_trackUsageFuture = m_apiService.TrackUsageAsync(*it->second.websiteUrl, m_configService.GetOrCreateFrameworkInstanceId(), m_sessionId, env.GetFrameworkInfo().buildHash, *it->second.version, env.GetGameInfo().name, env.GetGameInfo().version, currentPlugins, logs);
        m_lastUsageTrackTime = std::chrono::steady_clock::now();
        m_hasInitialTrackingSent = true;
    }


    void CommunicationManager::OnRequestTrackUsage(const Events::System::OnRequestTrackUsage& e) {
        RequestTrackUsage();
    }

    void CommunicationManager::OnErrorReportSinkChanged(std::shared_ptr<Logging::Sinks::ErrorReportSink> newSink) {
        m_errorSink = newSink;
    }

} // namespace Modules
SPF_NS_END
