#pragma once

#include "SPF/Logging/Logger.hpp"
#include "SPF/Telemetry/GameContext.hpp"
#include "SPF/Telemetry/SCS/Controls.hpp"
#include "SPF/Telemetry/Sdk.hpp"

namespace SPF::Telemetry {
/**
 * @class ControlsProcessor
 * @brief Processes and owns user input and effective control values.
 */
class ControlsProcessor {
 public:
  ControlsProcessor(Logging::Logger& logger, GameContext& context);

  void Initialize(const scs_telemetry_init_params_v100_t* const scs_params);
  void Shutdown();

  void HandleChannelUpdate(const scs_string_t name, const scs_u32_t index, const scs_value_t* value);

  const SCS::Controls& GetData() const { return m_controls; }

 private:
  Logging::Logger& m_logger;
  GameContext& m_context;

  SCS::Controls m_controls;
};

}  // namespace SPF::Telemetry
