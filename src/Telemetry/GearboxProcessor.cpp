#include "SPF/Telemetry/GearboxProcessor.hpp"

#include <cstring>

#include "SPF/Logging/Logger.hpp"
#include "SPF/Telemetry/GameContext.hpp"
#include "SPF/Telemetry/ConfigAttributeReader.hpp"

SPF_NS_BEGIN
namespace Telemetry {

GearboxProcessor::GearboxProcessor(Logging::Logger& logger, GameContext& context) : m_logger(logger), m_context(context) {}

void GearboxProcessor::Initialize(const scs_telemetry_init_params_v100_t* const scs_params) { m_logger.Info("GearboxProcessor initialized."); }

void GearboxProcessor::Shutdown() { m_logger.Info("GearboxProcessor shut down."); }

void GearboxProcessor::HandleConfiguration(const scs_telemetry_configuration_t* info) {
  ConfigAttributeReader reader(info->attributes);

  if (strcmp(info->id, SCS_TELEMETRY_CONFIG_controls) == 0) {
    m_gearboxConstants.shifter_type = reader.GetString(SCS_TELEMETRY_CONFIG_ATTRIBUTE_shifter_type).value_or("N/A");
  } else if (strcmp(info->id, SCS_TELEMETRY_CONFIG_hshifter) == 0) {
    m_gearboxConstants.slot_gear.clear();
    m_gearboxConstants.slot_handle_position.clear();
    m_gearboxConstants.slot_selectors.clear();

    for (uint32_t i = 0;; ++i) {
      auto gear = reader.GetS32(SCS_TELEMETRY_CONFIG_ATTRIBUTE_slot_gear, i);
      if (!gear) break;  // No more slots

      m_gearboxConstants.slot_gear.push_back(gear.value());
      m_gearboxConstants.slot_handle_position.push_back(reader.GetU32(SCS_TELEMETRY_CONFIG_ATTRIBUTE_slot_handle_position, i).value_or(0));
      m_gearboxConstants.slot_selectors.push_back(reader.GetU32(SCS_TELEMETRY_CONFIG_ATTRIBUTE_slot_selectors, i).value_or(0));
    }
  }
}

}  // namespace Telemetry
SPF_NS_END
