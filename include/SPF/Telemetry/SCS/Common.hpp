#pragma once

#include <cstdint>
#include <ctime>  // For time_t
#include <string>
#include <vector>

#include "SPF/Namespace.hpp"
#include "SPF/Types.hpp"
#include "SPF/Telemetry/Sdk.hpp"

SPF_NS_BEGIN
namespace Telemetry {
namespace SCS {
// --- Game State & Time ---

struct Timestamps {
  uint64_t simulation = 0;
  uint64_t render = 0;
  uint64_t paused_simulation = 0;
};

struct GameState {
  bool paused = false;
  Game game_id = Game::Unknown;
  std::string game_name;
  uint32_t scs_game_version_major = 0;
  uint32_t scs_game_version_minor = 0;
  uint32_t telemetry_plugin_version_major = 0;
  uint32_t telemetry_plugin_version_minor = 0;
  uint32_t telemetry_game_version_major = 0;
  uint32_t telemetry_game_version_minor = 0;
  int32_t multiplayer_time_offset = 0;
  float scale = 1.0f;
};

struct GameDateTime {
  uint32_t DayOfWeek = 1;  // 1: Monday, 7: Sunday
  uint32_t Hour = 0;
  uint32_t Minute = 0;
};

struct CommonData {
  uint32_t game_time = 0;
  int32_t next_rest_stop = 0;
  GameDateTime next_rest_stop_time;
  float next_rest_stop_real_minutes = 0.0f;
  std::vector<std::string> substances;
};

// --- Wheels ---

struct WheelConstants {
  bool simulated = false;
  bool powered = false;
  bool steerable = false;
  bool liftable = false;
  float radius = 0.0f;
  scs_value_fvector_t position = {0.0f, 0.0f, 0.0f};
};

struct WheelData {
  bool on_ground = false;
  float suspension_deflection = 0.0f;
  float angular_velocity = 0.0f;
  float steering = 0.0f;
  float rotation = 0.0f;
  float lift = 0.0f;
  float lift_offset = 0.0f;
  uint32_t substance = 0;
};

}  // namespace SCS
}  // namespace Telemetry
SPF_NS_END
