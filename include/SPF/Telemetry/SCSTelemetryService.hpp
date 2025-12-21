#pragma once

#include <memory>
#include <vector>

#include "SPF/Namespace.hpp"
#include "SPF/Modules/ITelemetryService.hpp"
#include "SPF/Utils/Signal.hpp"
#include "SPF/Telemetry/SCS/Gearbox.hpp"
#include "SPF/Telemetry/Sdk.hpp"
#include <chrono>

SPF_NS_BEGIN

// Forward declarations
namespace Logging {
class Logger;
}
namespace Events {
class EventManager;
}
namespace Telemetry {
class GameContext;
// Forward declare all the processor classes
class GameDataProcessor;
class TruckProcessor;
class TrailerProcessor;
class JobProcessor;
class EventsProcessor;
class ControlsProcessor;
class GearboxProcessor;
}  // namespace Telemetry

namespace Telemetry {
/**
 * @class SCSTelemetryService
 * @brief The public-facing implementation of the ITelemetryService for SCS SDK.
 *
 * This class acts as a router, delegating SDK events and data processing
 * to specialized processor classes.
 */
class SCSTelemetryService final : public Modules::ITelemetryService {
 public:
  SCSTelemetryService(Logging::Logger& logger, GameContext& context, Events::EventManager& eventManager);
  ~SCSTelemetryService();

  // --- Service Lifecycle ---
  void Initialize(const scs_telemetry_init_params_t* const params);
  void Shutdown();

  // --- ITelemetryService Implementation ---
  const SCS::GameState& GetGameState() const override;
  const SCS::Timestamps& GetTimestamps() const override;
  const SCS::CommonData& GetCommonData() const override;
  const SCS::TruckConstants& GetTruckConstants() const override;
  const SCS::TruckData& GetTruckData() const override;
  const std::vector<SCS::Trailer>& GetTrailers() const override;
  const SCS::JobConstants& GetJobConstants() const override;
  const SCS::JobData& GetJobData() const override;
  const SCS::NavigationData& GetNavigationData() const override;
  const SCS::Controls& GetControls() const override;
  const SCS::SpecialEvents& GetSpecialEvents() const override;
  const SCS::GameplayEvents& GetGameplayEvents() const override;
  const SCS::GearboxConstants& GetGearboxConstants() const override;
  const std::string& GetLastGameplayEventId() const override;
  float GetDeltaTime() const override;

  // --- Signal Accessors (ITelemetryService Implementation) ---
  Utils::Signal<void(const SPF::Telemetry::SCS::GameState&)>& GetGameStateSignal() override;
  Utils::Signal<void(const SPF::Telemetry::SCS::Timestamps&)>& GetTimestampsSignal() override;
  Utils::Signal<void(const SPF::Telemetry::SCS::CommonData&)>& GetCommonDataSignal() override;
  Utils::Signal<void(const SPF::Telemetry::SCS::TruckConstants&)>& GetTruckConstantsSignal() override;
  Utils::Signal<void(const SPF::Telemetry::SCS::TrailerConstants&)>& GetTrailerConstantsSignal() override;
  Utils::Signal<void(const SPF::Telemetry::SCS::TruckData&)>& GetTruckDataSignal() override;
  Utils::Signal<void(const std::vector<SPF::Telemetry::SCS::Trailer>&)>& GetTrailersSignal() override;
  Utils::Signal<void(const SPF::Telemetry::SCS::JobConstants&)>& GetJobConstantsSignal() override;
  Utils::Signal<void(const SPF::Telemetry::SCS::JobData&)>& GetJobDataSignal() override;
  Utils::Signal<void(const SPF::Telemetry::SCS::NavigationData&)>& GetNavigationDataSignal() override;
  Utils::Signal<void(const SPF::Telemetry::SCS::Controls&)>& GetControlsSignal() override;
  Utils::Signal<void(const SPF::Telemetry::SCS::SpecialEvents&)>& GetSpecialEventsSignal() override;
  Utils::Signal<void(const char*, const SPF::Telemetry::SCS::GameplayEvents&)>& GetGameplayEventsSignal() override;
  Utils::Signal<void(const SPF::Telemetry::SCS::GearboxConstants&)>& GetGearboxConstantsSignal() override;

  // --- Static Callbacks for SCS SDK ---
  static void StaticConfigurationCallback(scs_event_t event, const void* event_info, scs_context_t context);
  static void StaticFrameStartCallback(scs_event_t event, const void* event_info, scs_context_t context);
  static void StaticPausedCallback(scs_event_t event, const void* event_info, scs_context_t context);
  static void StaticStartedCallback(scs_event_t event, const void* event_info, scs_context_t context);
  static void StaticGameplayEventCallback(scs_event_t event, const void* event_info, scs_context_t context);
  static void StaticChannelCallback(const scs_string_t name, const scs_u32_t index, const scs_value_t* value, scs_context_t context);

 private:
  // --- Internal Event Handlers (Routers) ---
  void HandleConfiguration(const scs_telemetry_configuration_t* info);
  void HandleFrameStart(const scs_telemetry_frame_start_t* info);
  void HandleGameplayEvent(const scs_telemetry_gameplay_event_t* info);
  void HandlePaused();
  void HandleStarted();
  void HandleChannelUpdate(const scs_string_t name, const scs_u32_t index, const scs_value_t* value);

  // --- Dynamic Channel Registration ---
  void UpdateTruckWheelChannels(scs_u32_t wheel_count);
  void UpdateTrailerWheelChannels(scs_u32_t trailer_index, scs_u32_t wheel_count);
  void UpdateHShifterSelectorChannels(scs_u32_t selector_count);

  // --- Processors ---
  std::unique_ptr<GameDataProcessor> m_gameDataProcessor;
  std::unique_ptr<TruckProcessor> m_truckProcessor;
  std::unique_ptr<TrailerProcessor> m_trailerProcessor;
  std::unique_ptr<JobProcessor> m_jobProcessor;
  std::unique_ptr<EventsProcessor> m_eventsProcessor;
  std::unique_ptr<ControlsProcessor> m_controlsProcessor;
  std::unique_ptr<GearboxProcessor> m_gearboxProcessor;

  // Common dependencies passed to processors
  Logging::Logger& m_logger;
  GameContext& m_context;
  Events::EventManager& m_eventManager;

  // SDK function pointers for dynamic channel registration
  scs_telemetry_register_for_channel_t m_register_for_channel = nullptr;
  scs_telemetry_unregister_from_channel_t m_unregister_from_channel = nullptr;

  // Tracking for dynamic channel registration
  scs_u32_t m_registered_truck_wheel_count = 0;
  scs_u32_t m_registered_hshifter_selector_count = 0;
  std::vector<scs_u32_t> m_registered_trailer_wheel_counts;

  // Delta time calculation
  float m_deltaTime = 0.0f;
  std::chrono::steady_clock::time_point m_lastFrameTime;
};

}  // namespace Telemetry
SPF_NS_END
