#pragma once

#include "SPF/SPF_API/SPF_Telemetry_API.h"
#include "SPF/Namespace.hpp"
#include "SPF/Telemetry/SCS/Common.hpp"   // For GameState, Timestamps, CommonData
#include "SPF/Telemetry/SCS/Truck.hpp"    // For TruckConstants, TruckData
#include "SPF/Telemetry/SCS/Trailer.hpp"  // For Trailer, TrailerConstants (used in std::vector<Trailer>)
#include "SPF/Telemetry/SCS/Job.hpp"      // For JobConstants, JobData
#include "SPF/Telemetry/SCS/Navigation.hpp" // For NavigationData
#include "SPF/Telemetry/SCS/Controls.hpp" // For Controls
#include "SPF/Telemetry/SCS/Events.hpp"   // For SpecialEvents, GameplayEvents
#include "SPF/Telemetry/SCS/Gearbox.hpp"  // For GearboxConstants

#include "SPF/Utils/Signal.hpp"
#include "SPF/Utils/Delegate.hpp"
#include <functional> // For std::function
#include <memory> // For std::unique_ptr, std::make_unique

SPF_NS_BEGIN

// Forward declaration of EventManager needed for SubscriptionHandler constructor
namespace Events {
class EventManager;
}
namespace Modules::API {
class TelemetryApi {
 public:
    // Base class for all telemetry subscription handlers. Used for type erasure in TelemetryHandle.
    struct BaseSubscriptionHandler {
        virtual ~BaseSubscriptionHandler() = default;
    };

    // Templated handler for specific telemetry event types
    template <typename CppDataType>
    struct SubscriptionHandler : public BaseSubscriptionHandler {
        using InvokerFunction = std::function<void(const CppDataType&, void* user_data_ptr)>;

        SubscriptionHandler(
            Utils::Signal<void(const CppDataType&)>& signal, // The specific signal from EventManager
            InvokerFunction invoker_func, // Function that performs conversion and calls plugin callback
            void* user_data_ptr
        ) : m_invoker_func(invoker_func), m_user_data_ptr(user_data_ptr), m_sink(signal)
        {
            m_sink.template Connect<&SubscriptionHandler<CppDataType>::OnEvent>(this);
        }

        void OnEvent(const CppDataType& cpp_data) {
            m_invoker_func(cpp_data, m_user_data_ptr);
        }

        InvokerFunction m_invoker_func;
        void* m_user_data_ptr;
        Utils::Sink<void(const CppDataType&)> m_sink;
    };

    // Specialized handler for GameplayEvents
    struct GameplayEventSubscriptionHandler : public BaseSubscriptionHandler {
        using InvokerFunction = std::function<void(const char*, const SPF::Telemetry::SCS::GameplayEvents&, void* user_data_ptr)>;

        GameplayEventSubscriptionHandler(
            Utils::Signal<void(const char*, const SPF::Telemetry::SCS::GameplayEvents&)>& signal,
            InvokerFunction invoker_func,
            void* user_data_ptr
        ) : m_invoker_func(invoker_func), m_user_data_ptr(user_data_ptr), m_sink(signal)
        {
            m_sink.template Connect<&GameplayEventSubscriptionHandler::OnEvent>(this);
        }

        void OnEvent(const char* event_id, const SPF::Telemetry::SCS::GameplayEvents& cpp_data) {
            m_invoker_func(event_id, cpp_data, m_user_data_ptr);
        }

        InvokerFunction m_invoker_func;
        void* m_user_data_ptr;
        Utils::Sink<void(const char*, const SPF::Telemetry::SCS::GameplayEvents&)> m_sink;
    };

  static void FillTelemetryApi(SPF_Telemetry_API* api);

  // --- Event-Driven Callback Invocation & Conversion ---
  // These functions are called by PluginManager to perform data conversion and invoke plugin callbacks.
  static void InvokeGameStateCallback(const SPF::Telemetry::SCS::GameState& cpp_data, SPF_Telemetry_GameState_Callback callback, void* user_data);
  static void InvokeTimestampsCallback(const SPF::Telemetry::SCS::Timestamps& cpp_data, SPF_Telemetry_Timestamps_Callback callback, void* user_data);
  static void InvokeCommonDataCallback(const SPF::Telemetry::SCS::CommonData& cpp_data, SPF_Telemetry_CommonData_Callback callback, void* user_data);
  static void InvokeTruckConstantsCallback(const SPF::Telemetry::SCS::TruckConstants& cpp_data, SPF_Telemetry_TruckConstants_Callback callback, void* user_data);
  static void InvokeTrailerConstantsCallback(const SPF::Telemetry::SCS::TrailerConstants& cpp_data, SPF_Telemetry_TrailerConstants_Callback callback, void* user_data);
  static void InvokeTruckDataCallback(const SPF::Telemetry::SCS::TruckData& cpp_data, SPF_Telemetry_TruckData_Callback callback, void* user_data);
  static void InvokeTrailersCallback(const std::vector<SPF::Telemetry::SCS::Trailer>& cpp_data, SPF_Telemetry_Trailers_Callback callback, void* user_data);
  static void InvokeJobConstantsCallback(const SPF::Telemetry::SCS::JobConstants& cpp_data, SPF_Telemetry_JobConstants_Callback callback, void* user_data);
  static void InvokeJobDataCallback(const SPF::Telemetry::SCS::JobData& cpp_data, SPF_Telemetry_JobData_Callback callback, void* user_data);
  static void InvokeNavigationDataCallback(const SPF::Telemetry::SCS::NavigationData& cpp_data, SPF_Telemetry_NavigationData_Callback callback, void* user_data);
  static void InvokeControlsCallback(const SPF::Telemetry::SCS::Controls& cpp_data, SPF_Telemetry_Controls_Callback callback, void* user_data);
  static void InvokeSpecialEventsCallback(const SPF::Telemetry::SCS::SpecialEvents& cpp_data, SPF_Telemetry_SpecialEvents_Callback callback, void* user_data);
  static void InvokeGameplayEventsCallback(const char* event_id, const SPF::Telemetry::SCS::GameplayEvents& cpp_data, SPF_Telemetry_GameplayEvents_Callback callback, void* user_data);
  static void InvokeGearboxConstantsCallback(const SPF::Telemetry::SCS::GearboxConstants& cpp_data, SPF_Telemetry_GearboxConstants_Callback callback, void* user_data);

  // --- Event Subscription (New RAII-based C-API Proxies) ---
  static SPF_Telemetry_Callback_Handle* T_RegisterForGameState(SPF_Telemetry_Handle* handle, SPF_Telemetry_GameState_Callback callback, void* user_data);
  static SPF_Telemetry_Callback_Handle* T_RegisterForTimestamps(SPF_Telemetry_Handle* handle, SPF_Telemetry_Timestamps_Callback callback, void* user_data);
  static SPF_Telemetry_Callback_Handle* T_RegisterForCommonData(SPF_Telemetry_Handle* handle, SPF_Telemetry_CommonData_Callback callback, void* user_data);
  static SPF_Telemetry_Callback_Handle* T_RegisterForTruckConstants(SPF_Telemetry_Handle* handle, SPF_Telemetry_TruckConstants_Callback callback, void* user_data);
  static SPF_Telemetry_Callback_Handle* T_RegisterForTrailerConstants(SPF_Telemetry_Handle* handle, SPF_Telemetry_TrailerConstants_Callback callback, void* user_data);
  static SPF_Telemetry_Callback_Handle* T_RegisterForTruckData(SPF_Telemetry_Handle* handle, SPF_Telemetry_TruckData_Callback callback, void* user_data);
  static SPF_Telemetry_Callback_Handle* T_RegisterForTrailers(SPF_Telemetry_Handle* handle, SPF_Telemetry_Trailers_Callback callback, void* user_data);
  static SPF_Telemetry_Callback_Handle* T_RegisterForJobConstants(SPF_Telemetry_Handle* handle, SPF_Telemetry_JobConstants_Callback callback, void* user_data);
  static SPF_Telemetry_Callback_Handle* T_RegisterForJobData(SPF_Telemetry_Handle* handle, SPF_Telemetry_JobData_Callback callback, void* user_data);
  static SPF_Telemetry_Callback_Handle* T_RegisterForNavigationData(SPF_Telemetry_Handle* handle, SPF_Telemetry_NavigationData_Callback callback, void* user_data);
  static SPF_Telemetry_Callback_Handle* T_RegisterForControls(SPF_Telemetry_Handle* handle, SPF_Telemetry_Controls_Callback callback, void* user_data);
  static SPF_Telemetry_Callback_Handle* T_RegisterForSpecialEvents(SPF_Telemetry_Handle* handle, SPF_Telemetry_SpecialEvents_Callback callback, void* user_data);
  static SPF_Telemetry_Callback_Handle* T_RegisterForGameplayEvents(SPF_Telemetry_Handle* handle, SPF_Telemetry_GameplayEvents_Callback callback, void* user_data);
  static SPF_Telemetry_Callback_Handle* T_RegisterForGearboxConstants(SPF_Telemetry_Handle* handle, SPF_Telemetry_GearboxConstants_Callback callback, void* user_data);

 private:
  static SPF_Telemetry_Handle* T_GetContext(const char* pluginName);
  static void T_GetGameState(SPF_Telemetry_Handle* handle, SPF_GameState* out_data);
  static void T_GetTimestamps(SPF_Telemetry_Handle* handle, SPF_Timestamps* out_data);
  static void T_GetCommonData(SPF_Telemetry_Handle* handle, SPF_CommonData* out_data);
  static void T_GetTruckConstants(SPF_Telemetry_Handle* handle, SPF_TruckConstants* out_data);
  static void T_GetTruckData(SPF_Telemetry_Handle* handle, SPF_TruckData* out_data);
  static void T_GetTrailers(SPF_Telemetry_Handle* handle, SPF_Trailer* out_trailers, uint32_t* in_out_count);
  static void T_GetJobConstants(SPF_Telemetry_Handle* handle, SPF_JobConstants* out_data);
  static void T_GetJobData(SPF_Telemetry_Handle* handle, SPF_JobData* out_data);
  static void T_GetNavigationData(SPF_Telemetry_Handle* handle, SPF_NavigationData* out_data);
  static void T_GetControls(SPF_Telemetry_Handle* handle, SPF_Controls* out_data);
  static void T_GetSpecialEvents(SPF_Telemetry_Handle* handle, SPF_SpecialEvents* out_data);
  static void T_GetGameplayEvents(SPF_Telemetry_Handle* handle, SPF_GameplayEvents* out_data);
  static void T_GetGearboxConstants(SPF_Telemetry_Handle* handle, SPF_GearboxConstants* out_data);
  static int T_GetLastGameplayEventId(SPF_Telemetry_Handle* handle, char* out_buffer, int buffer_size);



};
}  // namespace Modules::API
SPF_NS_END
