#include "SPF/Telemetry/SCSTelemetryService.hpp"

#include <cstring>

#include "SPF/Logging/Logger.hpp"
#include "SPF/Telemetry/GameContext.hpp"
#include "SPF/Telemetry/GameDataProcessor.hpp"
#include "SPF/Telemetry/TruckProcessor.hpp"
#include "SPF/Telemetry/TrailerProcessor.hpp"
#include "SPF/Telemetry/JobProcessor.hpp"
#include "SPF/Telemetry/EventsProcessor.hpp"
#include "SPF/Telemetry/ControlsProcessor.hpp"
#include "SPF/Telemetry/GearboxProcessor.hpp"
#include "SPF/Events/EventManager.hpp"
#include "SPF/Events/TelemetryEvents.hpp"
#include "SPF/Telemetry/ConfigAttributeReader.hpp"

SPF_NS_BEGIN
namespace Telemetry {
using namespace Modules;

SCSTelemetryService::SCSTelemetryService(Logging::Logger& logger, GameContext& context, Events::EventManager& eventManager)
    : m_logger(logger), m_context(context), m_eventManager(eventManager) {
  m_gameDataProcessor = std::make_unique<GameDataProcessor>(logger, context, m_eventManager);
  m_truckProcessor = std::make_unique<TruckProcessor>(logger, context);
  m_trailerProcessor = std::make_unique<TrailerProcessor>(logger, context);
  m_jobProcessor = std::make_unique<JobProcessor>(logger, context);
  m_eventsProcessor = (std::make_unique<EventsProcessor>(logger, context));
  m_controlsProcessor = (std::make_unique<ControlsProcessor>(logger, context));
  m_gearboxProcessor = (std::make_unique<GearboxProcessor>(logger, context));
  m_lastFrameTime = (std::chrono::steady_clock::now());

  m_logger.Info("SCSTelemetryService and all its processors created.");
}

SCSTelemetryService::~SCSTelemetryService() { m_logger.Info("SCSTelemetryService destroyed."); }

void SCSTelemetryService::Initialize(const scs_telemetry_init_params_t* const params) {
  m_logger.Info("SCSTelemetryService initializing and registering callbacks...");
  const auto* versioned_params = static_cast<const scs_telemetry_init_params_v100_t*>(params);

  // Store SDK functions for later use
  m_register_for_channel = versioned_params->register_for_channel;
  m_unregister_from_channel = versioned_params->unregister_from_channel;
  m_registered_trailer_wheel_counts.resize(SCS_TELEMETRY_trailers_count, 0);

  m_gameDataProcessor->Initialize(versioned_params);
  m_truckProcessor->Initialize(versioned_params);
  m_trailerProcessor->Initialize(versioned_params);
  m_jobProcessor->Initialize(versioned_params);
  m_eventsProcessor->Initialize(versioned_params);
  m_controlsProcessor->Initialize(versioned_params);
  m_gearboxProcessor->Initialize(versioned_params);

  // Register for events.
  auto registerForEvent = versioned_params->register_for_event;
  if (registerForEvent) {
    registerForEvent(SCS_TELEMETRY_EVENT_configuration, StaticConfigurationCallback, this);
    registerForEvent(SCS_TELEMETRY_EVENT_frame_start, StaticFrameStartCallback, this);
    registerForEvent(SCS_TELEMETRY_EVENT_paused, StaticPausedCallback, this);
    registerForEvent(SCS_TELEMETRY_EVENT_started, StaticStartedCallback, this);
    registerForEvent(SCS_TELEMETRY_EVENT_gameplay, StaticGameplayEventCallback, this);
  }

  // Register for all channels explicitly.
  auto registerForChannel = versioned_params->register_for_channel;
  if (registerForChannel) {
    m_logger.Info("Registering all telemetry channels...");

    // Common Channels
    registerForChannel(SCS_TELEMETRY_CHANNEL_local_scale, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_CHANNEL_game_time, SCS_U32_NIL, SCS_VALUE_TYPE_u32, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_CHANNEL_multiplayer_time_offset, SCS_U32_NIL, SCS_VALUE_TYPE_s32, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_CHANNEL_next_rest_stop, SCS_U32_NIL, SCS_VALUE_TYPE_s32, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);

    // Job Channels
    registerForChannel(SCS_TELEMETRY_JOB_CHANNEL_cargo_damage, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);

    // Truck Channels (including Controls and Navigation)
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_world_placement, SCS_U32_NIL, SCS_VALUE_TYPE_dplacement, SCS_TELEMETRY_CHANNEL_FLAG_each_frame, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_local_linear_velocity, SCS_U32_NIL, SCS_VALUE_TYPE_fvector, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_local_angular_velocity, SCS_U32_NIL, SCS_VALUE_TYPE_fvector, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_local_linear_acceleration, SCS_U32_NIL, SCS_VALUE_TYPE_fvector, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_local_angular_acceleration, SCS_U32_NIL, SCS_VALUE_TYPE_fvector, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_cabin_offset, SCS_U32_NIL, SCS_VALUE_TYPE_fplacement, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_cabin_angular_velocity, SCS_U32_NIL, SCS_VALUE_TYPE_fvector, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_cabin_angular_acceleration, SCS_U32_NIL, SCS_VALUE_TYPE_fvector, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_head_offset, SCS_U32_NIL, SCS_VALUE_TYPE_fplacement, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_speed, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_engine_rpm, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_engine_gear, SCS_U32_NIL, SCS_VALUE_TYPE_s32, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_displayed_gear, SCS_U32_NIL, SCS_VALUE_TYPE_s32, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_input_steering, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_input_throttle, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_input_brake, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_input_clutch, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_effective_steering, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_effective_throttle, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_effective_brake, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_effective_clutch, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_cruise_control, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_hshifter_slot, SCS_U32_NIL, SCS_VALUE_TYPE_u32, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_parking_brake, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_motor_brake, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_retarder_level, SCS_U32_NIL, SCS_VALUE_TYPE_u32, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_brake_air_pressure, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_brake_air_pressure_warning, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_brake_air_pressure_emergency, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_brake_temperature, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_fuel, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_fuel_warning, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_fuel_average_consumption, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_fuel_range, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);

    if (strcmp(versioned_params->common.game_id, SCS_GAME_ID_EUT2) == 0) {
      registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_adblue, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
      registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_adblue_warning, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
      //registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_adblue_average_consumption, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    }

    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_oil_pressure, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_oil_pressure_warning, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_oil_temperature, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_water_temperature, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_water_temperature_warning, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_battery_voltage, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_battery_voltage_warning, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_electric_enabled, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_engine_enabled, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_wipers, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_differential_lock, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_lift_axle, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_lift_axle_indicator, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_trailer_lift_axle, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_trailer_lift_axle_indicator, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_lblinker, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_rblinker, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_hazard_warning, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_light_lblinker, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_light_rblinker, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_light_parking, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_light_low_beam, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_light_high_beam, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_light_aux_front, SCS_U32_NIL, SCS_VALUE_TYPE_u32, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_light_aux_roof, SCS_U32_NIL, SCS_VALUE_TYPE_u32, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_light_beacon, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_light_brake, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_light_reverse, SCS_U32_NIL, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_dashboard_backlight, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_wear_engine, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_wear_transmission, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_wear_cabin, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_wear_chassis, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_wear_wheels, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_odometer, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_navigation_distance, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_navigation_time, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    registerForChannel(SCS_TELEMETRY_TRUCK_CHANNEL_navigation_speed_limit, SCS_U32_NIL, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
  }
}

void SCSTelemetryService::UpdateTrailerWheelChannels(scs_u32_t trailer_index, scs_u32_t wheel_count) {
  if (!m_register_for_channel || !m_unregister_from_channel) return;
  if (trailer_index >= m_registered_trailer_wheel_counts.size()) return;

  char channel_name[128];
  scs_u32_t& registered_count = m_registered_trailer_wheel_counts[trailer_index];

  // Unregister
  while (registered_count > wheel_count) {
    --registered_count;
    sprintf(channel_name, "trailer.%u.wheel.suspension.deflection", trailer_index);
    m_unregister_from_channel(channel_name, registered_count, SCS_VALUE_TYPE_float);
    sprintf(channel_name, "trailer.%u.wheel.on_ground", trailer_index);
    m_unregister_from_channel(channel_name, registered_count, SCS_VALUE_TYPE_bool);
    sprintf(channel_name, "trailer.%u.wheel.substance", trailer_index);
    m_unregister_from_channel(channel_name, registered_count, SCS_VALUE_TYPE_u32);
    sprintf(channel_name, "trailer.%u.wheel.angular_velocity", trailer_index);
    m_unregister_from_channel(channel_name, registered_count, SCS_VALUE_TYPE_float);
    sprintf(channel_name, "trailer.%u.wheel.steering", trailer_index);
    m_unregister_from_channel(channel_name, registered_count, SCS_VALUE_TYPE_float);
    sprintf(channel_name, "trailer.%u.wheel.rotation", trailer_index);
    m_unregister_from_channel(channel_name, registered_count, SCS_VALUE_TYPE_float);
    sprintf(channel_name, "trailer.%u.wheel.lift", trailer_index);
    m_unregister_from_channel(channel_name, registered_count, SCS_VALUE_TYPE_float);
    sprintf(channel_name, "trailer.%u.wheel.lift.offset", trailer_index);
    m_unregister_from_channel(channel_name, registered_count, SCS_VALUE_TYPE_float);
  }

  // Register
  while (registered_count < wheel_count) {
    sprintf(channel_name, "trailer.%u.wheel.suspension.deflection", trailer_index);
    m_register_for_channel(channel_name, registered_count, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    sprintf(channel_name, "trailer.%u.wheel.on_ground", trailer_index);
    m_register_for_channel(channel_name, registered_count, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    sprintf(channel_name, "trailer.%u.wheel.substance", trailer_index);
    m_register_for_channel(channel_name, registered_count, SCS_VALUE_TYPE_u32, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    sprintf(channel_name, "trailer.%u.wheel.angular_velocity", trailer_index);
    m_register_for_channel(channel_name, registered_count, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    sprintf(channel_name, "trailer.%u.wheel.steering", trailer_index);
    m_register_for_channel(channel_name, registered_count, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    sprintf(channel_name, "trailer.%u.wheel.rotation", trailer_index);
    m_register_for_channel(channel_name, registered_count, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    sprintf(channel_name, "trailer.%u.wheel.lift", trailer_index);
    m_register_for_channel(channel_name, registered_count, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    sprintf(channel_name, "trailer.%u.wheel.lift.offset", trailer_index);
    m_register_for_channel(channel_name, registered_count, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    ++registered_count;
  }
}

void SCSTelemetryService::UpdateTruckWheelChannels(scs_u32_t wheel_count) {
  if (!m_register_for_channel || !m_unregister_from_channel) return;

  // Unregister channels for wheels that no longer exist.
  while (m_registered_truck_wheel_count > wheel_count) {
    --m_registered_truck_wheel_count;
    m_unregister_from_channel(SCS_TELEMETRY_TRUCK_CHANNEL_wheel_susp_deflection, m_registered_truck_wheel_count, SCS_VALUE_TYPE_float);
    m_unregister_from_channel(SCS_TELEMETRY_TRUCK_CHANNEL_wheel_on_ground, m_registered_truck_wheel_count, SCS_VALUE_TYPE_bool);
    m_unregister_from_channel(SCS_TELEMETRY_TRUCK_CHANNEL_wheel_substance, m_registered_truck_wheel_count, SCS_VALUE_TYPE_u32);
    m_unregister_from_channel(SCS_TELEMETRY_TRUCK_CHANNEL_wheel_velocity, m_registered_truck_wheel_count, SCS_VALUE_TYPE_float);
    m_unregister_from_channel(SCS_TELEMETRY_TRUCK_CHANNEL_wheel_steering, m_registered_truck_wheel_count, SCS_VALUE_TYPE_float);
    m_unregister_from_channel(SCS_TELEMETRY_TRUCK_CHANNEL_wheel_rotation, m_registered_truck_wheel_count, SCS_VALUE_TYPE_float);
    m_unregister_from_channel(SCS_TELEMETRY_TRUCK_CHANNEL_wheel_lift, m_registered_truck_wheel_count, SCS_VALUE_TYPE_float);
    m_unregister_from_channel(SCS_TELEMETRY_TRUCK_CHANNEL_wheel_lift_offset, m_registered_truck_wheel_count, SCS_VALUE_TYPE_float);
  }

  // Register channels for new wheels.
  while (m_registered_truck_wheel_count < wheel_count) {
    m_register_for_channel(
        SCS_TELEMETRY_TRUCK_CHANNEL_wheel_susp_deflection, m_registered_truck_wheel_count, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    m_register_for_channel(
        SCS_TELEMETRY_TRUCK_CHANNEL_wheel_on_ground, m_registered_truck_wheel_count, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    m_register_for_channel(
        SCS_TELEMETRY_TRUCK_CHANNEL_wheel_substance, m_registered_truck_wheel_count, SCS_VALUE_TYPE_u32, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    m_register_for_channel(
        SCS_TELEMETRY_TRUCK_CHANNEL_wheel_velocity, m_registered_truck_wheel_count, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    m_register_for_channel(
        SCS_TELEMETRY_TRUCK_CHANNEL_wheel_steering, m_registered_truck_wheel_count, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    m_register_for_channel(
        SCS_TELEMETRY_TRUCK_CHANNEL_wheel_rotation, m_registered_truck_wheel_count, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    m_register_for_channel(
        SCS_TELEMETRY_TRUCK_CHANNEL_wheel_lift, m_registered_truck_wheel_count, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    m_register_for_channel(
        SCS_TELEMETRY_TRUCK_CHANNEL_wheel_lift_offset, m_registered_truck_wheel_count, SCS_VALUE_TYPE_float, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    ++m_registered_truck_wheel_count;
  }
}

void SCSTelemetryService::UpdateHShifterSelectorChannels(scs_u32_t selector_count) {
  if (!m_register_for_channel || !m_unregister_from_channel) return;

  // Unregister channels for selectors that no longer exist.
  while (m_registered_hshifter_selector_count > selector_count) {
    --m_registered_hshifter_selector_count;
    m_unregister_from_channel(SCS_TELEMETRY_TRUCK_CHANNEL_hshifter_selector, m_registered_hshifter_selector_count, SCS_VALUE_TYPE_bool);
  }

  // Register channels for new selectors.
  while (m_registered_hshifter_selector_count < selector_count) {
    m_register_for_channel(
        SCS_TELEMETRY_TRUCK_CHANNEL_hshifter_selector, m_registered_hshifter_selector_count, SCS_VALUE_TYPE_bool, SCS_TELEMETRY_CHANNEL_FLAG_none, StaticChannelCallback, this);
    ++m_registered_hshifter_selector_count;
  }
}

void SCSTelemetryService::Shutdown() {
  m_logger.Info("SCSTelemetryService shutting down.");
  // TODO: Unregister channels
  m_controlsProcessor->Shutdown();
  m_eventsProcessor->Shutdown();
  m_jobProcessor->Shutdown();
  m_trailerProcessor->Shutdown();
  m_truckProcessor->Shutdown();
  m_gameDataProcessor->Shutdown();
}

// --- Static Callbacks & Routers ---

void SCSTelemetryService::StaticConfigurationCallback(scs_event_t, const void* event_info, scs_context_t context) {
  if (context) static_cast<SCSTelemetryService*>(context)->HandleConfiguration(static_cast<const scs_telemetry_configuration_t*>(event_info));
}

void SCSTelemetryService::HandleConfiguration(const scs_telemetry_configuration_t* info) {
  if (!info || !info->id) return;

  if (strcmp(info->id, SCS_TELEMETRY_CONFIG_truck) == 0) {
    m_truckProcessor->HandleConfiguration(info);
    UpdateTruckWheelChannels(m_truckProcessor->GetConstants().wheel_count);
    UpdateHShifterSelectorChannels(m_truckProcessor->GetConstants().selector_count);

    // Notify about the truck configuration update.
    m_eventManager.System.Telemetry.OnTruckConstantsChanged.Call(m_truckProcessor->GetConstants());
  } else if (strncmp(info->id, "trailer.", 8) == 0) {
    m_trailerProcessor->HandleConfiguration(info);
    unsigned int trailer_index;
    if (sscanf_s(info->id, "trailer.%u", &trailer_index) == 1 && trailer_index < m_trailerProcessor->GetData().size()) {
      UpdateTrailerWheelChannels(trailer_index, m_trailerProcessor->GetData()[trailer_index].constants.wheel_count);

      // Notify about the trailer configuration update.
      m_eventManager.System.Telemetry.OnTrailerConstantsChanged.Call(m_trailerProcessor->GetData()[trailer_index].constants);
    }
  } else if (strcmp(info->id, SCS_TELEMETRY_CONFIG_trailer) == 0) {
    m_trailerProcessor->HandleConfiguration(info);
    if (!m_trailerProcessor->GetData().empty()) {
      UpdateTrailerWheelChannels(0, m_trailerProcessor->GetData()[0].constants.wheel_count);

      // Notify about the trailer configuration update (for the first trailer).
      m_eventManager.System.Telemetry.OnTrailerConstantsChanged.Call(m_trailerProcessor->GetData()[0].constants);
    }
  } else if (strcmp(info->id, SCS_TELEMETRY_CONFIG_job) == 0) {
    m_jobProcessor->HandleConfiguration(info);

    // Notify about the job configuration update.
    m_eventManager.System.Telemetry.OnJobConstantsChanged.Call(m_jobProcessor->GetJobConstants());
  } else if (strcmp(info->id, SCS_TELEMETRY_CONFIG_substances) == 0) {
    m_gameDataProcessor->HandleConfiguration(info);

  m_eventManager.System.Telemetry.OnCommonDataUpdated.Call(m_gameDataProcessor->GetCommonData());
  } else if (strcmp(info->id, SCS_TELEMETRY_CONFIG_controls) == 0 || strcmp(info->id, SCS_TELEMETRY_CONFIG_hshifter) == 0) {
    m_gearboxProcessor->HandleConfiguration(info);

    // Notify about the gearbox/controls configuration update.
    m_eventManager.System.Telemetry.OnGearboxConstantsChanged.Call(m_gearboxProcessor->GetConstants());
  }
}

void SCSTelemetryService::StaticChannelCallback(const scs_string_t name, const scs_u32_t index, const scs_value_t* value, scs_context_t context) {
  if (context) static_cast<SCSTelemetryService*>(context)->HandleChannelUpdate(name, index, value);
}

void SCSTelemetryService::HandleChannelUpdate(const scs_string_t name, const scs_u32_t index, const scs_value_t* value) {
  if (!name || !value) return;

  // Route channel data to the appropriate processor based on the channel name prefix.
  else if (strncmp(name, "truck.input.", 12) == 0 || strncmp(name, "truck.effective.", 16) == 0) {
    m_controlsProcessor->HandleChannelUpdate(name, index, value);
  } else if (strncmp(name, "truck.navigation.", 17) == 0)  // Route truck.navigation.* channels to JobProcessor
  {
    m_jobProcessor->HandleChannelUpdate(name, index, value);
  } else if (strncmp(name, "truck.", 6) == 0) {
    m_truckProcessor->HandleChannelUpdate(name, index, value);
  } else if (strncmp(name, "trailer.", 8) == 0) {
    m_trailerProcessor->HandleChannelUpdate(name, index, value);
  } else if (strncmp(name, "job.", 4) == 0) {
    m_jobProcessor->HandleChannelUpdate(name, index, value);
  } else {
    m_gameDataProcessor->HandleChannelUpdate(name, index, value);
  }
}

void SCSTelemetryService::StaticFrameStartCallback(scs_event_t, const void* event_info, scs_context_t context) {
  if (context) static_cast<SCSTelemetryService*>(context)->HandleFrameStart(static_cast<const scs_telemetry_frame_start_t*>(event_info));
}

void SCSTelemetryService::HandleFrameStart(const scs_telemetry_frame_start_t* info) {
  // Calculate delta time first
  auto currentTime = std::chrono::steady_clock::now();
  std::chrono::duration<float> dt_duration = currentTime - m_lastFrameTime;
  m_deltaTime = dt_duration.count();
  m_lastFrameTime = currentTime;

  m_gameDataProcessor->HandleFrameStart(info);

  // Post-process calculations that depend on multiple processors
  const auto& jobConstants = m_jobProcessor->GetJobConstants();
  if (jobConstants.delivery_time > 0)  // Job is active
  {
    const auto& commonData = m_gameDataProcessor->GetCommonData();
    auto& jobData = m_jobProcessor->GetMutableJobData();

    if (jobConstants.delivery_time < commonData.game_time) {
      jobData.remaining_delivery_minutes = 0;
    } else {
      jobData.remaining_delivery_minutes = jobConstants.delivery_time - commonData.game_time;
    }
  }

  // Calculate real-time to destination
  auto& navData = m_jobProcessor->GetMutableNavigationData();
  const auto& gameState = m_gameDataProcessor->GetGameState();
  if (gameState.scale > 0.0f && navData.navigation_time > 0.0f) {
    navData.navigation_time_real_seconds = navData.navigation_time / gameState.scale;
  } else {
    navData.navigation_time_real_seconds = 0.0f;
  }

  // --- Fire Data Update Events ---
  // Now that all channel data for the frame has been processed, notify listeners
  // with the complete, updated data structures.
  m_eventManager.System.Telemetry.OnTimestampsUpdated.Call(m_gameDataProcessor->GetTimestamps());
  m_eventManager.System.Telemetry.OnTruckDataUpdated.Call(m_truckProcessor->GetData());
  m_eventManager.System.Telemetry.OnTrailersUpdated.Call(m_trailerProcessor->GetData());
  m_eventManager.System.Telemetry.OnJobDataUpdated.Call(m_jobProcessor->GetJobData());
  m_eventManager.System.Telemetry.OnNavigationDataUpdated.Call(m_jobProcessor->GetNavigationData());
  m_eventManager.System.Telemetry.OnControlsUpdated.Call(m_controlsProcessor->GetData());
  m_eventManager.System.Telemetry.OnCommonDataUpdated.Call(m_gameDataProcessor->GetCommonData());

  // Notify the system that a telemetry frame has started
  m_eventManager.System.OnTelemetryFrameStart.Call();

  // Reset single-frame event flags AFTER plugins have had a chance to process them.
  m_eventsProcessor->HandleFrameStart();
}

void SCSTelemetryService::StaticPausedCallback(scs_event_t, const void*, scs_context_t context) {
  if (context) static_cast<SCSTelemetryService*>(context)->HandlePaused();
}

void SCSTelemetryService::HandlePaused() {
  m_gameDataProcessor->HandlePaused();
  m_eventManager.System.Telemetry.OnGameStateUpdated.Call(m_gameDataProcessor->GetGameState());
}

void SCSTelemetryService::StaticStartedCallback(scs_event_t, const void*, scs_context_t context) {
  if (context) static_cast<SCSTelemetryService*>(context)->HandleStarted();
}

void SCSTelemetryService::HandleStarted() {
  m_gameDataProcessor->HandleStarted();
  m_eventManager.System.Telemetry.OnGameStateUpdated.Call(m_gameDataProcessor->GetGameState());
}

void SCSTelemetryService::StaticGameplayEventCallback(scs_event_t, const void* event_info, scs_context_t context) {
  if (context) static_cast<SCSTelemetryService*>(context)->HandleGameplayEvent(static_cast<const scs_telemetry_gameplay_event_t*>(event_info));
}

void SCSTelemetryService::HandleGameplayEvent(const scs_telemetry_gameplay_event_t* info) {
  // Let the processor handle the raw event first to update its internal state.
  m_eventsProcessor->HandleGameplayEvent(info);

  // Now, fire the framework-level event with the processed data.
  const char* event_id = m_eventsProcessor->GetLastGameplayEventId().c_str();
  const auto& event_data = m_eventsProcessor->GetGameplayEvents();

  m_eventManager.System.Telemetry.OnGameplayEventsUpdated.Call(event_id, event_data);
  m_eventManager.System.Telemetry.OnSpecialEventsUpdated.Call(m_eventsProcessor->GetSpecialEvents());
}

// --- ITelemetryService Implementation ---

const SCS::GameState& SCSTelemetryService::GetGameState() const { return m_gameDataProcessor->GetGameState(); }
const SCS::Timestamps& SCSTelemetryService::GetTimestamps() const { return m_gameDataProcessor->GetTimestamps(); }
const SCS::CommonData& SCSTelemetryService::GetCommonData() const { return m_gameDataProcessor->GetCommonData(); }
const SCS::TruckConstants& SCSTelemetryService::GetTruckConstants() const { return m_truckProcessor->GetConstants(); }
const SCS::TruckData& SCSTelemetryService::GetTruckData() const { return m_truckProcessor->GetData(); }
const std::vector<SCS::Trailer>& SCSTelemetryService::GetTrailers() const { return m_trailerProcessor->GetData(); }
const SCS::JobConstants& SCSTelemetryService::GetJobConstants() const { return m_jobProcessor->GetJobConstants(); }
const SCS::JobData& SCSTelemetryService::GetJobData() const { return m_jobProcessor->GetJobData(); }
const SCS::NavigationData& SCSTelemetryService::GetNavigationData() const { return m_jobProcessor->GetNavigationData(); }
const SCS::Controls& SCSTelemetryService::GetControls() const { return m_controlsProcessor->GetData(); }
const SCS::SpecialEvents& SCSTelemetryService::GetSpecialEvents() const { return m_eventsProcessor->GetSpecialEvents(); }
const SCS::GameplayEvents& SCSTelemetryService::GetGameplayEvents() const { return m_eventsProcessor->GetGameplayEvents(); }
const SCS::GearboxConstants& SCSTelemetryService::GetGearboxConstants() const { return m_gearboxProcessor->GetConstants(); }

const std::string& SCSTelemetryService::GetLastGameplayEventId() const { return m_eventsProcessor->GetLastGameplayEventId(); }

float SCSTelemetryService::GetDeltaTime() const { return m_deltaTime; }

// --- Signal Accessors (ITelemetryService Implementation) ---
Utils::Signal<void(const SCS::GameState&)>& SCSTelemetryService::GetGameStateSignal() {
    return m_eventManager.System.Telemetry.OnGameStateUpdated;
}

Utils::Signal<void(const SCS::Timestamps&)>& SCSTelemetryService::GetTimestampsSignal() {
    return m_eventManager.System.Telemetry.OnTimestampsUpdated;
}

Utils::Signal<void(const SCS::CommonData&)>& SCSTelemetryService::GetCommonDataSignal() {
    return m_eventManager.System.Telemetry.OnCommonDataUpdated;
}

Utils::Signal<void(const SCS::TruckConstants&)>& SCSTelemetryService::GetTruckConstantsSignal() {
    return m_eventManager.System.Telemetry.OnTruckConstantsChanged;
}

Utils::Signal<void(const SCS::TrailerConstants&)>& SCSTelemetryService::GetTrailerConstantsSignal() {
    return m_eventManager.System.Telemetry.OnTrailerConstantsChanged;
}

Utils::Signal<void(const SCS::TruckData&)>& SCSTelemetryService::GetTruckDataSignal() {
    return m_eventManager.System.Telemetry.OnTruckDataUpdated;
}

Utils::Signal<void(const std::vector<SCS::Trailer>&)>& SCSTelemetryService::GetTrailersSignal() {
    return m_eventManager.System.Telemetry.OnTrailersUpdated;
}

Utils::Signal<void(const SCS::JobConstants&)>& SCSTelemetryService::GetJobConstantsSignal() {
    return m_eventManager.System.Telemetry.OnJobConstantsChanged;
}

Utils::Signal<void(const SCS::JobData&)>& SCSTelemetryService::GetJobDataSignal() {
    return m_eventManager.System.Telemetry.OnJobDataUpdated;
}

Utils::Signal<void(const SCS::NavigationData&)>& SCSTelemetryService::GetNavigationDataSignal() {
    return m_eventManager.System.Telemetry.OnNavigationDataUpdated;
}

Utils::Signal<void(const SCS::Controls&)>& SCSTelemetryService::GetControlsSignal() {
    return m_eventManager.System.Telemetry.OnControlsUpdated;
}

Utils::Signal<void(const SCS::SpecialEvents&)>& SCSTelemetryService::GetSpecialEventsSignal() {
    return m_eventManager.System.Telemetry.OnSpecialEventsUpdated;
}

Utils::Signal<void(const char*, const SCS::GameplayEvents&)>& SCSTelemetryService::GetGameplayEventsSignal() {
    return m_eventManager.System.Telemetry.OnGameplayEventsUpdated;
}

Utils::Signal<void(const SCS::GearboxConstants&)>& SCSTelemetryService::GetGearboxConstantsSignal() {
    return m_eventManager.System.Telemetry.OnGearboxConstantsChanged;
}

}  // namespace Telemetry
SPF_NS_END