#pragma once

#include "SPF/Telemetry/SCS/Common.hpp"

#include "scssdk_value.h"

#include <cstdint>
#include <string>
#include <vector>

namespace SPF::Telemetry::SCS {

struct TrailerConstants {
  std::string id;
  std::string cargo_accessory_id;
  std::string brand_id;
  std::string brand;
  std::string name;
  std::string chain_type;
  std::string body_type;
  std::string license_plate;
  std::string license_plate_country_id;
  std::string license_plate_country;
  scs_value_fvector_t hook_position = {};
  uint32_t wheel_count = 0;
  std::vector<WheelConstants> wheels;
};

struct TrailerData {
  bool connected = false;
  scs_value_dplacement_t world_placement = {};
  scs_value_fvector_t local_linear_velocity = {};
  scs_value_fvector_t local_angular_velocity = {};
  scs_value_fvector_t local_linear_acceleration = {};
  scs_value_fvector_t local_angular_acceleration = {};
  float cargo_damage = 0.0f;
  float wear_body = 0.0f;
  float wear_chassis = 0.0f;
  float wear_wheels = 0.0f;
  std::vector<WheelData> wheels;
};

struct Trailer {
  TrailerConstants constants;
  TrailerData data;
  bool channels_registered = false;  // Internal flag for the processor
};
}  // namespace SPF::Telemetry::SCS
