#pragma once

#include "SPF/Namespace.hpp"
#include "SPF/Telemetry/SCS/Truck.hpp"
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
/**
 * @class TruckProcessor
 * @brief Processes and owns all truck-related telemetry data.
 */
class TruckProcessor {
 public:
  TruckProcessor(Logging::Logger& logger, GameContext& context);

  void Initialize(const scs_telemetry_init_params_v100_t* const scs_params);
  void Shutdown();

  void HandleConfiguration(const scs_telemetry_configuration_t* info);
  void HandleChannelUpdate(const scs_string_t name, const scs_u32_t index, const scs_value_t* value);

  const SCS::TruckData& GetData() const { return m_truckData; }
  const SCS::TruckConstants& GetConstants() const { return m_truckConstants; }
  SCS::TruckConstants& GetMutableConstants() { return m_truckConstants; }

 private:
  Logging::Logger& m_logger;
  GameContext& m_context;

  SCS::TruckData m_truckData;
  SCS::TruckConstants m_truckConstants;
};

}  // namespace Telemetry
SPF_NS_END
