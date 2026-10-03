#include "SPF/Telemetry/JobProcessor.hpp"

#include "SPF/Logging/Logger.hpp"
#include "SPF/Telemetry/ConfigAttributeReader.hpp"
#include "SPF/Telemetry/GameContext.hpp"
#include "SPF/Telemetry/Sdk.hpp"

#include <cstring>

namespace SPF::Telemetry {

JobProcessor::JobProcessor(Logging::Logger& logger, GameContext& context) : m_logger(logger), m_context(context) {}

void JobProcessor::Initialize(const scs_telemetry_init_params_v100_t* const scs_params) { m_logger.Info("JobProcessor initialized."); }

void JobProcessor::Shutdown() { m_logger.Info("JobProcessor shut down."); }

void JobProcessor::HandleConfiguration(const scs_telemetry_configuration_t* info) {
  if (strcmp(info->id, SCS_TELEMETRY_CONFIG_car_job) == 0) {
    m_logger.Info("JobProcessor handling car job configuration...");

    ConfigAttributeReader reader(info->attributes);

    m_carJobConstants.income = reader.GetU64(SCS_TELEMETRY_CONFIG_ATTRIBUTE_income).value_or(0);
    m_carJobConstants.delivery_time = reader.GetU32(SCS_TELEMETRY_CONFIG_ATTRIBUTE_delivery_time).value_or(0);
    m_carJobConstants.planned_distance_km = reader.GetU32(SCS_TELEMETRY_CONFIG_ATTRIBUTE_planned_distance_km).value_or(0);
    m_carJobConstants.car_job_market = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_car_job_market).value_or("");

    m_carJobConstants.customer_prio_cargo_handling = reader.GetBool(SCS_TELEMETRY_CONFIG_ATTRIBUTE_customer_prio_cargo_handling).value_or(false);
    m_carJobConstants.customer_prio_time = reader.GetBool(SCS_TELEMETRY_CONFIG_ATTRIBUTE_customer_prio_time).value_or(false);
    m_carJobConstants.customer_prio_vehicle_appearance = reader.GetBool(SCS_TELEMETRY_CONFIG_ATTRIBUTE_customer_prio_vehicle_appearance).value_or(false);

    m_carJobConstants.cargo_id = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_cargo_id).value_or("");
    m_carJobConstants.cargo_name = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_cargo).value_or("");
    m_carJobConstants.cargo_unit_count = reader.GetU32(SCS_TELEMETRY_CONFIG_ATTRIBUTE_cargo_unit_count).value_or(0);

    m_carJobConstants.destination_city_id = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_destination_city_id).value_or("");
    m_carJobConstants.destination_city = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_destination_city).value_or("");
    m_carJobConstants.destination_company_id = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_destination_company_id).value_or("");
    m_carJobConstants.destination_company = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_destination_company).value_or("");

    m_carJobConstants.source_city_id = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_source_city_id).value_or("");
    m_carJobConstants.source_city = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_source_city).value_or("");
    m_carJobConstants.source_company_id = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_source_company_id).value_or("");
    m_carJobConstants.source_company = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_source_company).value_or("");

    return;
  }

  if (strcmp(info->id, SCS_TELEMETRY_CONFIG_job) != 0) {
    return;  // Not a job config.
  }

  m_logger.Info("JobProcessor handling job configuration...");

  ConfigAttributeReader reader(info->attributes);

  m_jobConstants.income = reader.GetU64(SCS_TELEMETRY_CONFIG_ATTRIBUTE_income).value_or(0);
  m_jobConstants.delivery_time = reader.GetU32(SCS_TELEMETRY_CONFIG_ATTRIBUTE_delivery_time).value_or(0);
  m_jobConstants.planned_distance_km = reader.GetU32(SCS_TELEMETRY_CONFIG_ATTRIBUTE_planned_distance_km).value_or(0);
  m_jobConstants.is_cargo_loaded = reader.GetBool(SCS_TELEMETRY_CONFIG_ATTRIBUTE_is_cargo_loaded).value_or(false);
  m_jobConstants.is_special_job = reader.GetBool(SCS_TELEMETRY_CONFIG_ATTRIBUTE_special_job).value_or(false);
  m_jobConstants.job_market = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_job_market).value_or("");

  m_jobConstants.cargo_id = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_cargo_id).value_or("");
  m_jobConstants.cargo_name = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_cargo).value_or("");
  m_jobConstants.cargo_mass = reader.GetFloat(SCS_TELEMETRY_CONFIG_ATTRIBUTE_cargo_mass).value_or(0.0f);
  m_jobConstants.cargo_unit_count = reader.GetU32(SCS_TELEMETRY_CONFIG_ATTRIBUTE_cargo_unit_count).value_or(0);
  m_jobConstants.cargo_unit_mass = reader.GetFloat(SCS_TELEMETRY_CONFIG_ATTRIBUTE_cargo_unit_mass).value_or(0.0f);

  m_jobConstants.destination_city_id = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_destination_city_id).value_or("");
  m_jobConstants.destination_city = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_destination_city).value_or("");
  m_jobConstants.destination_company_id = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_destination_company_id).value_or("");
  m_jobConstants.destination_company = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_destination_company).value_or("");

  m_jobConstants.source_city_id = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_source_city_id).value_or("");
  m_jobConstants.source_city = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_source_city).value_or("");
  m_jobConstants.source_company_id = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_source_company_id).value_or("");
  m_jobConstants.source_company = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_source_company).value_or("");

  // After reading, update the on_job status
  m_jobData.on_job = !m_jobConstants.job_market.empty();
}

void JobProcessor::HandleChannelUpdate(const scs_string_t name, const scs_u32_t index, const scs_value_t* value) {
  if (!value || !name) return;

  if (strcmp(name, SCS_TELEMETRY_JOB_CHANNEL_cargo_damage) == 0) {
    m_jobData.cargo_damage = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_navigation_distance) == 0)  // Note: SDK uses truck channel for this
  {
    m_navigationData.navigation_distance = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_navigation_time) == 0) {
    m_navigationData.navigation_time = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_navigation_speed_limit) == 0) {
    m_navigationData.navigation_speed_limit = value->value_float.value;
  }
}

}  // namespace SPF::Telemetry
