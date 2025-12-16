#include "SPF/Telemetry/ControlsProcessor.hpp"

#include <cstring>

#include "SPF/Logging/Logger.hpp"
#include "SPF/Telemetry/GameContext.hpp"

// SDK headers for channel names
#include "common/scssdk_telemetry_truck_common_channels.h"

SPF_NS_BEGIN
namespace Telemetry {

ControlsProcessor::ControlsProcessor(Logging::Logger& logger, GameContext& context) : m_logger(logger), m_context(context) {}

void ControlsProcessor::Initialize(const scs_telemetry_init_params_v100_t* const scs_params) { m_logger.Info("ControlsProcessor initialized."); }

void ControlsProcessor::Shutdown() { m_logger.Info("ControlsProcessor shut down."); }

void ControlsProcessor::HandleChannelUpdate(const scs_string_t name, const scs_u32_t index, const scs_value_t* value) {
  if (!value || !name) return;

  // User Input
  if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_input_steering) == 0) {
    m_controls.userInput.steering = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_input_throttle) == 0) {
    m_controls.userInput.throttle = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_input_brake) == 0) {
    m_controls.userInput.brake = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_input_clutch) == 0) {
    m_controls.userInput.clutch = value->value_float.value;
  }
  // Effective Input
  else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_effective_steering) == 0) {
    m_controls.effectiveInput.steering = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_effective_throttle) == 0) {
    m_controls.effectiveInput.throttle = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_effective_brake) == 0) {
    m_controls.effectiveInput.brake = value->value_float.value;
  } else if (strcmp(name, SCS_TELEMETRY_TRUCK_CHANNEL_effective_clutch) == 0) {
    m_controls.effectiveInput.clutch = value->value_float.value;
  }
}

}  // namespace Telemetry
SPF_NS_END
