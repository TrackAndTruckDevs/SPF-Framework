#pragma once

#include "SPF/Namespace.hpp"
#include "SPF/Telemetry/SCS/Gearbox.hpp"
#include "SPF/Telemetry/Sdk.hpp"

SPF_NS_BEGIN

// Forward declarations
namespace Logging {
class Logger;
}
namespace Telemetry {
class GameContext;
}

namespace Telemetry {
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

}  // namespace Telemetry
SPF_NS_END
