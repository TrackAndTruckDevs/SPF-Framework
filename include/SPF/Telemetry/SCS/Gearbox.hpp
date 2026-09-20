#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace SPF::Telemetry::SCS {

struct GearboxConstants {
  std::string shifter_type;

  // H-Shifter layout
  std::vector<int32_t> slot_gear;
  std::vector<uint32_t> slot_handle_position;
  std::vector<uint32_t> slot_selectors;
};
}  // namespace SPF::Telemetry::SCS
