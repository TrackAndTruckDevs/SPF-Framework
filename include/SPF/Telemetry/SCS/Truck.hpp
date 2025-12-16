#pragma once

#include <string>
#include <vector>

#include "SPF/Namespace.hpp"
#include "SPF/Telemetry/SCS/Common.hpp"

SPF_NS_BEGIN
namespace Telemetry {
namespace SCS {
struct TruckConstants {
  std::string id;
  std::string brand_id;
  std::string brand;
  std::string name;
  std::string license_plate;
  std::string license_plate_country_id;
  std::string license_plate_country;

  float fuel_capacity = 0.0f;
  float fuel_warning_factor = 0.0f;
  float adblue_capacity = 0.0f;
  float adblue_warning_factor = 0.0f;

  float air_pressure_warning = 0.0f;
  float air_pressure_emergency = 0.0f;
  float oil_pressure_warning = 0.0f;
  float water_temperature_warning = 0.0f;
  float battery_voltage_warning = 0.0f;

  float rpm_limit = 0.0f;
  uint32_t forward_gear_count = 0;
  uint32_t reverse_gear_count = 0;
  uint32_t retarder_step_count = 0;
  uint32_t selector_count = 0;
  float differential_ratio = 0.0f;

  scs_value_fvector_t cabin_position = {};
  scs_value_fvector_t head_position = {};
  scs_value_fvector_t hook_position = {};

  uint32_t wheel_count = 0;
  std::vector<float> gear_ratios_forward;
  std::vector<float> gear_ratios_reverse;
  std::vector<WheelConstants> wheels;
};

struct TruckData {
  // Physics
  scs_value_dplacement_t world_placement = {};
  scs_value_fvector_t local_linear_velocity = {};
  scs_value_fvector_t local_angular_velocity = {};
  scs_value_fvector_t local_linear_acceleration = {};
  scs_value_fvector_t local_angular_acceleration = {};
  scs_value_fplacement_t cabin_offset = {};
  scs_value_fvector_t cabin_angular_velocity = {};
  scs_value_fvector_t cabin_angular_acceleration = {};
  scs_value_fplacement_t head_offset = {};

  float speed = 0.0f;
  float engine_rpm = 0.0f;
  int32_t gear = 0;
  int32_t displayed_gear = 0;

  float input_steering = 0.0f;
  float input_throttle = 0.0f;
  float input_brake = 0.0f;
  float input_clutch = 0.0f;

  float effective_steering = 0.0f;
  float effective_throttle = 0.0f;
  float effective_brake = 0.0f;
  float effective_clutch = 0.0f;

  float cruise_control_speed = 0.0f;
  bool parking_brake = false;
  bool motor_brake = false;
  uint32_t retarder_level = 0;
  float air_pressure = 0.0f;
  bool air_pressure_warning = false;
  bool air_pressure_emergency = false;
  float brake_temperature = 0.0f;

  float fuel_amount = 0.0f;
  bool fuel_warning = false;
  float fuel_average_consumption = 0.0f;
  float fuel_range = 0.0f;
  float adblue_amount = 0.0f;
  bool adblue_warning = false;
  float adblue_average_consumption = 0.0f;

  float oil_pressure = 0.0f;
  bool oil_pressure_warning = false;
  float oil_temperature = 0.0f;
  float water_temperature = 0.0f;
  bool water_temperature_warning = false;
  float battery_voltage = 0.0f;
  bool battery_voltage_warning = false;

  bool electric_enabled = false;
  bool engine_enabled = false;
  bool wipers = false;
  bool differential_lock = false;
  bool lift_axle = false;
  bool lift_axle_indicator = false;
  bool trailer_lift_axle = false;
  bool trailer_lift_axle_indicator = false;

  bool lblinker = false;
  bool rblinker = false;
  bool hazard_warning = false;
  bool light_lblinker = false;
  bool light_rblinker = false;
  bool light_parking = false;
  bool light_low_beam = false;
  bool light_high_beam = false;
  uint32_t light_aux_front = 0;
  uint32_t light_aux_roof = 0;
  bool light_beacon = false;
  bool light_brake = false;
  bool light_reverse = false;
  float dashboard_backlight = 0.0f;

  float wear_engine = 0.0f;
  float wear_transmission = 0.0f;
  float wear_cabin = 0.0f;
  float wear_chassis = 0.0f;
  float wear_wheels = 0.0f;  // Average, for backward compatibility

  float odometer = 0.0f;
  uint32_t hshifter_slot = 0;
  std::vector<bool> hshifter_selector;

  std::vector<WheelData> wheels;
};
}  // namespace SCS
}  // namespace Telemetry
SPF_NS_END
