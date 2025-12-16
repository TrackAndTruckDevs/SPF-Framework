#include "SPF/Telemetry/TrailerProcessor.hpp"

#include <cstring>
#include <vector>
#include <cstdio>  // For sscanf

#include "SPF/Logging/Logger.hpp"
#include "SPF/Telemetry/ConfigAttributeReader.hpp"
#include "SPF/Telemetry/GameContext.hpp"

// SDK headers for channel and attribute names
#include "common/scssdk_telemetry_common_configs.h"
#include "common/scssdk_telemetry_trailer_common_channels.h"

SPF_NS_BEGIN
namespace Telemetry {

TrailerProcessor::TrailerProcessor(Logging::Logger& logger, GameContext& context) : m_logger(logger), m_context(context) {
  // Pre-allocate for the max number of trailers to avoid reallocations.
  m_trailers.resize(SCS_TELEMETRY_trailers_count);
}

void TrailerProcessor::Initialize(const scs_telemetry_init_params_v100_t* const scs_params) { m_logger.Info("TrailerProcessor initialized."); }

void TrailerProcessor::Shutdown() { m_logger.Info("TrailerProcessor shut down."); }

void TrailerProcessor::HandleConfiguration(const scs_telemetry_configuration_t* info) {
  // The info->id will be "trailer.0", "trailer.1", etc.
  unsigned int trailer_index;

  // Per SDK documentation (scssdk_telemetry_common_configs.h), we only process
  // indexed configurations like "trailer.0", "trailer.1", etc. The non-indexed "trailer"
  // config is for backward compatibility and is a duplicate of "trailer.0", so we ignore it
  // to prevent processing the same trailer twice.
  if (sscanf_s(info->id, "trailer.%u", &trailer_index) != 1) {
    // If the ID doesn't match the "trailer.INDEX" format, ignore it.
    return;
  }

  if (trailer_index >= m_trailers.size()) {
    m_logger.Warn("Received config for trailer index %u which is out of bounds (max %zu).", trailer_index, m_trailers.size());
    return;
  }

  auto& trailer = m_trailers[trailer_index];
  auto& constants = trailer.constants;
  ConfigAttributeReader reader(info->attributes);

  constants.id = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_id).value_or("");
  constants.cargo_accessory_id = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_cargo_accessory_id).value_or("");
  constants.brand_id = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_brand_id).value_or("");
  constants.brand = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_brand).value_or("");
  constants.name = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_name).value_or("");
  constants.chain_type = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_chain_type).value_or("");
  constants.body_type = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_body_type).value_or("");
  constants.license_plate = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_license_plate).value_or("");
  constants.license_plate_country_id = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_license_plate_country_id).value_or("");
  constants.license_plate_country = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_license_plate_country).value_or("");
  constants.hook_position = reader.GetFVector(SCS_TELEMETRY_CONFIG_ATTRIBUTE_hook_position).value_or(scs_value_fvector_t{});

  // Wheels
  const auto wheel_count = reader.GetU32(SCS_TELEMETRY_CONFIG_ATTRIBUTE_wheel_count).value_or(0);
  constants.wheel_count = wheel_count;

  // Dynamically resize wheel vectors to the actual count.
  trailer.data.wheels.resize(wheel_count);
  trailer.constants.wheels.resize(wheel_count);
  m_logger.Info("TrailerProcessor handling configuration for trailer {} (wheels: {})...", trailer_index, wheel_count);

  for (uint32_t i = 0; i < wheel_count; ++i) {
    auto& wheel = constants.wheels[i];
    wheel.simulated = reader.GetBool(SCS_TELEMETRY_CONFIG_ATTRIBUTE_wheel_simulated, i).value_or(false);
    wheel.powered = reader.GetBool(SCS_TELEMETRY_CONFIG_ATTRIBUTE_wheel_powered, i).value_or(false);
    wheel.steerable = reader.GetBool(SCS_TELEMETRY_CONFIG_ATTRIBUTE_wheel_steerable, i).value_or(false);
    wheel.liftable = reader.GetBool(SCS_TELEMETRY_CONFIG_ATTRIBUTE_wheel_liftable, i).value_or(false);
    wheel.radius = reader.GetFloat(SCS_TELEMETRY_CONFIG_ATTRIBUTE_wheel_radius, i).value_or(0.0f);
    wheel.position = reader.GetFVector(SCS_TELEMETRY_CONFIG_ATTRIBUTE_wheel_position, i).value_or(scs_value_fvector_t{});
  }
}

void TrailerProcessor::HandleChannelUpdate(const scs_string_t name, const scs_u32_t wheel_index, const scs_value_t* value) {
  if (!value || !name) return;

  unsigned int trailer_index = 0;
  const char* channel_part = nullptr;

  // Find the first dot.
  const char* first_dot = strchr(name, '.');
  if (!first_dot) return;  // Not a trailer channel

  // Find the second dot if it exists.
  const char* second_dot = strchr(first_dot + 1, '.');

  if (second_dot)  // Format is "trailer.INDEX.channel"
  {
    trailer_index = static_cast<unsigned int>(atoi(first_dot + 1));
    channel_part = second_dot + 1;
  } else  // Format is "trailer.channel" (backward compatibility for first trailer)
  {
    trailer_index = 0;
    channel_part = first_dot + 1;
  }

  if (trailer_index >= m_trailers.size()) return;

  auto& trailer_data = m_trailers[trailer_index].data;

  if (strcmp(channel_part, "connected") == 0) {
    trailer_data.connected = value->value_bool.value;
  } else if (strcmp(channel_part, "cargo.damage") == 0) {
    trailer_data.cargo_damage = value->value_float.value;
  } else if (strcmp(channel_part, "world.placement") == 0) {
    trailer_data.world_placement = value->value_dplacement;
  } else if (strcmp(channel_part, "velocity.linear") == 0) {
    trailer_data.local_linear_velocity = value->value_fvector;
  } else if (strcmp(channel_part, "velocity.angular") == 0) {
    trailer_data.local_angular_velocity = value->value_fvector;
  } else if (strcmp(channel_part, "acceleration.linear") == 0) {
    trailer_data.local_linear_acceleration = value->value_fvector;
  } else if (strcmp(channel_part, "acceleration.angular") == 0) {
    trailer_data.local_angular_acceleration = value->value_fvector;
  } else if (strcmp(channel_part, "wear.body") == 0) {
    trailer_data.wear_body = value->value_float.value;
  } else if (strcmp(channel_part, "wear.chassis") == 0) {
    trailer_data.wear_chassis = value->value_float.value;
  } else if (strcmp(channel_part, "wear.wheels") == 0) {
    trailer_data.wear_wheels = value->value_float.value;
  }
  // Wheel channels
  else if (strcmp(channel_part, "wheel.suspension.deflection") == 0) {
    if (wheel_index < trailer_data.wheels.size()) trailer_data.wheels[wheel_index].suspension_deflection = value->value_float.value;
  } else if (strcmp(channel_part, "wheel.on_ground") == 0) {
    if (wheel_index < trailer_data.wheels.size()) trailer_data.wheels[wheel_index].on_ground = value->value_bool.value;
  } else if (strcmp(channel_part, "wheel.substance") == 0) {
    if (wheel_index < trailer_data.wheels.size()) trailer_data.wheels[wheel_index].substance = value->value_u32.value;
  } else if (strcmp(channel_part, "wheel.angular_velocity") == 0) {
    if (wheel_index < trailer_data.wheels.size()) trailer_data.wheels[wheel_index].angular_velocity = value->value_float.value;
  } else if (strcmp(channel_part, "wheel.steering") == 0) {
    if (wheel_index < trailer_data.wheels.size()) trailer_data.wheels[wheel_index].steering = value->value_float.value;
  } else if (strcmp(channel_part, "wheel.rotation") == 0) {
    if (wheel_index < trailer_data.wheels.size()) trailer_data.wheels[wheel_index].rotation = value->value_float.value;
  } else if (strcmp(channel_part, "wheel.lift") == 0) {
    if (wheel_index < trailer_data.wheels.size()) trailer_data.wheels[wheel_index].lift = value->value_float.value;
  } else if (strcmp(channel_part, "wheel.lift.offset") == 0) {
    if (wheel_index < trailer_data.wheels.size()) trailer_data.wheels[wheel_index].lift_offset = value->value_float.value;
  }
}

}  // namespace Telemetry
SPF_NS_END
