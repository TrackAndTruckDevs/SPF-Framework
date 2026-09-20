#pragma once

namespace SPF::Telemetry::SCS {

struct NavigationData {
  float navigation_distance = 0.0f;
  float navigation_time = 0.0f;
  float navigation_speed_limit = 0.0f;
  float navigation_time_real_seconds = 0.0f;
};
}  // namespace SPF::Telemetry::SCS
