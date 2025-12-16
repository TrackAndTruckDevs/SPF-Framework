#pragma once

#include <string>
#include <cstdint>

#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Telemetry {
namespace SCS {
// Holds data for the 'job_cancelled' event
struct JobCancelledEvent {
  int64_t penalty = 0;
};

// Holds data for the 'job_delivered' event
struct JobDeliveredEvent {
  int64_t revenue = 0;
  int32_t earned_xp = 0;
  float cargo_damage = 0.0f;
  float distance_km = 0.0f;
  uint32_t delivery_time = 0;
  bool auto_park_used = false;
  bool auto_load_used = false;
};

// Holds data for the 'player_fined' event
struct PlayerFinedEvent {
  int64_t fine_amount = 0;
  std::string fine_offence;
};

// Holds data for the 'player_tollgate_paid' event
struct PlayerTollgatePaidEvent {
  int64_t pay_amount = 0;
};

// Holds data for ferry/train transport events
struct PlayerUseTransportEvent {
  int64_t pay_amount = 0;
  std::string source_name;
  std::string target_name;
  std::string source_id;
  std::string target_id;
};

// A container for all gameplay event data.
// Only the relevant struct will be populated when an event occurs.
struct GameplayEvents {
  JobCancelledEvent job_cancelled;
  JobDeliveredEvent job_delivered;
  PlayerFinedEvent player_fined;
  PlayerTollgatePaidEvent tollgate_paid;
  PlayerUseTransportEvent ferry_used;
  PlayerUseTransportEvent train_used;
};

// Contains boolean flags that are set to true for a single frame when an event occurs.
struct SpecialEvents {
  // Event flags
  bool job_cancelled = false;
  bool job_delivered = false;
  bool fined = false;
  bool tollgate = false;
  bool ferry = false;
  bool train = false;
};

}  // namespace SCS
}  // namespace Telemetry
SPF_NS_END
