#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Telemetry {
namespace SCS {
struct JobConstants {
  uint64_t income = 0;
  uint32_t delivery_time = 0;
  uint32_t planned_distance_km = 0;
  bool is_cargo_loaded = false;
  bool is_special_job = false;
  std::string job_market;
  std::string cargo_id;
  std::string cargo_name;
  float cargo_mass = 0.0f;
  uint32_t cargo_unit_count = 0;
  float cargo_unit_mass = 0.0f;
  std::string destination_city_id;
  std::string destination_city;
  std::string destination_company_id;
  std::string destination_company;
  std::string source_city_id;
  std::string source_city;
  std::string source_company_id;
  std::string source_company;
};

struct JobData {
  bool on_job = false;
  float cargo_damage = 0.0f;
  float remaining_delivery_minutes = 0.0f;
};
}  // namespace SCS
}  // namespace Telemetry
SPF_NS_END
