#include "SPF/Telemetry/TruckProcessor.hpp"

#include <cstring>
#include <vector>

#include "SPF/Logging/Logger.hpp"
#include "SPF/Telemetry/ConfigAttributeReader.hpp"
#include "SPF/Telemetry/GameContext.hpp"

// SDK headers for channel and attribute names
#include "common/scssdk_telemetry_common_configs.h"
#include "common/scssdk_telemetry_truck_common_channels.h"

SPF_NS_BEGIN
namespace Telemetry {
TruckProcessor::TruckProcessor(Logging::Logger& logger, GameContext& context) : m_logger(logger), m_context(context) {}

void TruckProcessor::Initialize(const scs_telemetry_init_params_v100_t* const scs_params) { m_logger.Info("TruckProcessor initialized."); }

void TruckProcessor::Shutdown() { m_logger.Info("TruckProcessor shut down."); }

void TruckProcessor::HandleConfiguration(const scs_telemetry_configuration_t* info) {
  m_logger.Info("TruckProcessor handling truck configuration...");

  ConfigAttributeReader reader(info->attributes);

  // Read all constant truck attributes.
  m_truckConstants.brand_id = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_brand_id).value_or("");
  m_truckConstants.brand = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_brand).value_or("");
  m_truckConstants.id = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_id).value_or("");
  m_truckConstants.name = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_name).value_or("");
  m_truckConstants.license_plate = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_license_plate).value_or("");
  m_truckConstants.license_plate_country_id = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_license_plate_country_id).value_or("");
  m_truckConstants.license_plate_country = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_license_plate_country).value_or("");

  m_truckConstants.fuel_capacity = reader.GetFloat(SCS_TELEMETRY_CONFIG_ATTRIBUTE_fuel_capacity).value_or(0.0f);
  m_truckConstants.fuel_warning_factor = reader.GetFloat(SCS_TELEMETRY_CONFIG_ATTRIBUTE_fuel_warning_factor).value_or(0.0f);
  m_truckConstants.adblue_capacity = reader.GetFloat(SCS_TELEMETRY_CONFIG_ATTRIBUTE_adblue_capacity).value_or(0.0f);
  m_truckConstants.adblue_warning_factor = reader.GetFloat(SCS_TELEMETRY_CONFIG_ATTRIBUTE_adblue_warning_factor).value_or(0.0f);

  m_truckConstants.air_pressure_warning = reader.GetFloat(SCS_TELEMETRY_CONFIG_ATTRIBUTE_air_pressure_warning).value_or(0.0f);
  m_truckConstants.air_pressure_emergency = reader.GetFloat(SCS_TELEMETRY_CONFIG_ATTRIBUTE_air_pressure_emergency).value_or(0.0f);
  m_truckConstants.oil_pressure_warning = reader.GetFloat(SCS_TELEMETRY_CONFIG_ATTRIBUTE_oil_pressure_warning).value_or(0.0f);
  m_truckConstants.water_temperature_warning = reader.GetFloat(SCS_TELEMETRY_CONFIG_ATTRIBUTE_water_temperature_warning).value_or(0.0f);
  m_truckConstants.battery_voltage_warning = reader.GetFloat(SCS_TELEMETRY_CONFIG_ATTRIBUTE_battery_voltage_warning).value_or(0.0f);

  m_truckConstants.rpm_limit = reader.GetFloat(SCS_TELEMETRY_CONFIG_ATTRIBUTE_rpm_limit).value_or(0.0f);
  m_truckConstants.forward_gear_count = reader.GetU32(SCS_TELEMETRY_CONFIG_ATTRIBUTE_forward_gear_count).value_or(0);
  m_truckConstants.reverse_gear_count = reader.GetU32(SCS_TELEMETRY_CONFIG_ATTRIBUTE_reverse_gear_count).value_or(0);
  m_truckConstants.retarder_step_count = reader.GetU32(SCS_TELEMETRY_CONFIG_ATTRIBUTE_retarder_step_count).value_or(0);
  m_truckConstants.differential_ratio = reader.GetFloat(SCS_TELEMETRY_CONFIG_ATTRIBUTE_differential_ratio).value_or(0.0f);

  m_truckConstants.cabin_position = reader.GetFVector(SCS_TELEMETRY_CONFIG_ATTRIBUTE_cabin_position).value_or(scs_value_fvector_t{});
  m_truckConstants.head_position = reader.GetFVector(SCS_TELEMETRY_CONFIG_ATTRIBUTE_head_position).value_or(scs_value_fvector_t{});
  m_truckConstants.hook_position = reader.GetFVector(SCS_TELEMETRY_CONFIG_ATTRIBUTE_hook_position).value_or(scs_value_fvector_t{});

  // Wheels
  const auto wheel_count = reader.GetU32(SCS_TELEMETRY_CONFIG_ATTRIBUTE_wheel_count).value_or(0);
  m_truckConstants.wheel_count = wheel_count;
  m_truckConstants.wheels.resize(wheel_count);
  m_truckData.wheels.resize(wheel_count);

  for (uint32_t i = 0; i < wheel_count; ++i) {
    auto& wheel = m_truckConstants.wheels[i];
    wheel.simulated = reader.GetBool(SCS_TELEMETRY_CONFIG_ATTRIBUTE_wheel_simulated, i).value_or(false);
    wheel.powered = reader.GetBool(SCS_TELEMETRY_CONFIG_ATTRIBUTE_wheel_powered, i).value_or(false);
    wheel.steerable = reader.GetBool(SCS_TELEMETRY_CONFIG_ATTRIBUTE_wheel_steerable, i).value_or(false);
    wheel.liftable = reader.GetBool(SCS_TELEMETRY_CONFIG_ATTRIBUTE_wheel_liftable, i).value_or(false);
    wheel.radius = reader.GetFloat(SCS_TELEMETRY_CONFIG_ATTRIBUTE_wheel_radius, i).value_or(0.0f);
    wheel.position = reader.GetFVector(SCS_TELEMETRY_CONFIG_ATTRIBUTE_wheel_position, i).value_or(scs_value_fvector_t{});
  }

  // Ratios
  m_truckConstants.gear_ratios_forward = reader.GetFloatArray(SCS_TELEMETRY_CONFIG_ATTRIBUTE_forward_ratio, m_truckConstants.forward_gear_count);
  m_truckConstants.gear_ratios_reverse = reader.GetFloatArray(SCS_TELEMETRY_CONFIG_ATTRIBUTE_reverse_ratio, m_truckConstants.reverse_gear_count);

  // H-Shifter selectors
  m_truckConstants.selector_count = reader.GetU32(SCS_TELEMETRY_CONFIG_ATTRIBUTE_selector_count).value_or(0);
  m_truckData.hshifter_selector.resize(m_truckConstants.selector_count);
}

void TruckProcessor::HandleChannelUpdate(const scs_string_t name, const scs_u32_t index, const scs_value_t* value) {
  if (!value || !name) return;

  // A big if-else if chain to handle all the channels.
  // This could be optimized with a map, but for clarity and performance, strcmp is fine.

  if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_world_placement) == 0) {
    m_truckData.world_placement = value->value_dplacement;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_local_linear_velocity) == 0) {
    m_truckData.local_linear_velocity = value->value_fvector;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_local_angular_velocity) == 0) {
    m_truckData.local_angular_velocity = value->value_fvector;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_local_linear_acceleration) == 0) {
    m_truckData.local_linear_acceleration = value->value_fvector;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_local_angular_acceleration) == 0) {
    m_truckData.local_angular_acceleration = value->value_fvector;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_cabin_offset) == 0) {
    m_truckData.cabin_offset = value->value_fplacement;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_cabin_angular_velocity) == 0) {
    m_truckData.cabin_angular_velocity = value->value_fvector;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_cabin_angular_acceleration) == 0) {
    m_truckData.cabin_angular_acceleration = value->value_fvector;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_head_offset) == 0) {
    m_truckData.head_offset = value->value_fplacement;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_speed) == 0) {
    m_truckData.speed = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_engine_rpm) == 0) {
    m_truckData.engine_rpm = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_engine_gear) == 0) {
    m_truckData.gear = value->value_s32.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_displayed_gear) == 0) {
    m_truckData.displayed_gear = value->value_s32.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_input_steering) == 0) {
    m_truckData.input_steering = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_input_throttle) == 0) {
    m_truckData.input_throttle = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_input_brake) == 0) {
    m_truckData.input_brake = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_input_clutch) == 0) {
    m_truckData.input_clutch = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_effective_steering) == 0) {
    m_truckData.effective_steering = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_effective_throttle) == 0) {
    m_truckData.effective_throttle = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_effective_brake) == 0) {
    m_truckData.effective_brake = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_effective_clutch) == 0) {
    m_truckData.effective_clutch = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_cruise_control) == 0) {
    m_truckData.cruise_control_speed = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_hshifter_slot) == 0) {
    m_truckData.hshifter_slot = value->value_u32.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_hshifter_selector) == 0) {
    if (index < m_truckData.hshifter_selector.size()) m_truckData.hshifter_selector[index] = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_parking_brake) == 0) {
    m_truckData.parking_brake = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_motor_brake) == 0) {
    m_truckData.motor_brake = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_retarder_level) == 0) {
    m_truckData.retarder_level = value->value_u32.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_brake_air_pressure) == 0) {
    m_truckData.air_pressure = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_brake_air_pressure_warning) == 0) {
    m_truckData.air_pressure_warning = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_brake_air_pressure_emergency) == 0) {
    m_truckData.air_pressure_emergency = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_brake_temperature) == 0) {
    m_truckData.brake_temperature = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_fuel) == 0) {
    m_truckData.fuel_amount = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_fuel_warning) == 0) {
    m_truckData.fuel_warning = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_fuel_average_consumption) == 0) {
    m_truckData.fuel_average_consumption = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_fuel_range) == 0) {
    m_truckData.fuel_range = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_adblue) == 0) {
    m_truckData.adblue_amount = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_adblue_warning) == 0) {
    m_truckData.adblue_warning = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_adblue_average_consumption) == 0) {
    m_truckData.adblue_average_consumption = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_oil_pressure) == 0) {
    m_truckData.oil_pressure = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_oil_pressure_warning) == 0) {
    m_truckData.oil_pressure_warning = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_oil_temperature) == 0) {
    m_truckData.oil_temperature = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_water_temperature) == 0) {
    m_truckData.water_temperature = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_water_temperature_warning) == 0) {
    m_truckData.water_temperature_warning = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_battery_voltage) == 0) {
    m_truckData.battery_voltage = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_battery_voltage_warning) == 0) {
    m_truckData.battery_voltage_warning = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_electric_enabled) == 0) {
    m_truckData.electric_enabled = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_engine_enabled) == 0) {
    m_truckData.engine_enabled = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_wipers) == 0) {
    m_truckData.wipers = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_differential_lock) == 0) {
    m_truckData.differential_lock = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_lift_axle) == 0) {
    m_truckData.lift_axle = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_lift_axle_indicator) == 0) {
    m_truckData.lift_axle_indicator = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_trailer_lift_axle) == 0) {
    m_truckData.trailer_lift_axle = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_trailer_lift_axle_indicator) == 0) {
    m_truckData.trailer_lift_axle_indicator = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_lblinker) == 0) {
    m_truckData.lblinker = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_rblinker) == 0) {
    m_truckData.rblinker = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_hazard_warning) == 0) {
    m_truckData.hazard_warning = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_light_lblinker) == 0) {
    m_truckData.light_lblinker = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_light_rblinker) == 0) {
    m_truckData.light_rblinker = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_light_parking) == 0) {
    m_truckData.light_parking = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_light_low_beam) == 0) {
    m_truckData.light_low_beam = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_light_high_beam) == 0) {
    m_truckData.light_high_beam = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_light_aux_front) == 0) {
    m_truckData.light_aux_front = value->value_u32.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_light_aux_roof) == 0) {
    m_truckData.light_aux_roof = value->value_u32.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_light_beacon) == 0) {
    m_truckData.light_beacon = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_light_brake) == 0) {
    m_truckData.light_brake = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_light_reverse) == 0) {
    m_truckData.light_reverse = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_dashboard_backlight) == 0) {
    m_truckData.dashboard_backlight = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_wear_engine) == 0) {
    m_truckData.wear_engine = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_wear_transmission) == 0) {
    m_truckData.wear_transmission = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_wear_cabin) == 0) {
    m_truckData.wear_cabin = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_wear_chassis) == 0) {
    m_truckData.wear_chassis = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_wear_wheels) == 0) {
    m_truckData.wear_wheels = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_odometer) == 0) {
    m_truckData.odometer = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_navigation_distance) == 0) {    /* Handled by JobProcessor */
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_navigation_time) == 0) {        /* Handled by JobProcessor */
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_navigation_speed_limit) == 0) { /* Handled by JobProcessor */
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_wheel_susp_deflection) == 0) {
    if (index < m_truckData.wheels.size()) m_truckData.wheels[index].suspension_deflection = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_wheel_on_ground) == 0) {
    if (index < m_truckData.wheels.size()) m_truckData.wheels[index].on_ground = value->value_bool.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_wheel_substance) == 0) {
    if (index < m_truckData.wheels.size()) m_truckData.wheels[index].substance = value->value_u32.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_wheel_velocity) == 0) {
    if (index < m_truckData.wheels.size()) m_truckData.wheels[index].angular_velocity = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_wheel_steering) == 0) {
    if (index < m_truckData.wheels.size()) m_truckData.wheels[index].steering = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_wheel_rotation) == 0) {
    if (index < m_truckData.wheels.size()) m_truckData.wheels[index].rotation = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_wheel_lift) == 0) {
    if (index < m_truckData.wheels.size()) m_truckData.wheels[index].lift = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_wheel_lift_offset) == 0) {
    if (index < m_truckData.wheels.size()) m_truckData.wheels[index].lift_offset = value->value_float.value;
  }
}

}  // namespace Telemetry
SPF_NS_END