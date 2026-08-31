#pragma once

#include "SPF/Logging/Logger.hpp"
#include "SPF/Telemetry/GameContext.hpp"
#include "SPF/Telemetry/SCS/Gearbox.hpp"
#include "SPF/Telemetry/Sdk.hpp"

namespace SPF::Telemetry {
class GearboxProcessor {
 public:
  GearboxProcessor(Logging::Logger& logger, GameContext& context);

  void Initialize(const scs_telemetry_init_params_v100_t* const scs_params);
  void Shutdown();

  void HandleConfiguration(const scs_telemetry_configuration_t* info);

  const SCS::GearboxConstants& GetConstants() const { return m_gearboxConstants; }

 private:
  Logging::Logger& m_logger;
  GameContext& m_context;

  SCS::GearboxConstants m_gearboxConstants;
};

}  // namespace SPF::Telemetry
