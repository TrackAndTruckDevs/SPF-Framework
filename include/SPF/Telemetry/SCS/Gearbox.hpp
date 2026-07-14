#pragma once

#include "SPF/Namespace.hpp"

#include <cstdint>
#include <string>
#include <vector>


SPF_NS_BEGIN
namespace Telemetry {
namespace SCS {
struct GearboxConstants {
  std::string shifter_type;

  // H-Shifter layout
  std::vector<int32_t> slot_gear;
  std::vector<uint32_t> slot_handle_position;
  std::vector<uint32_t> slot_selectors;
};
}  // namespace SCS
}  // namespace Telemetry
SPF_NS_END
