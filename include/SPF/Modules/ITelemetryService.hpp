#pragma once

#include <string>
#include <vector>

#include "SPF/Namespace.hpp"
#include "SPF/Telemetry/SCS/Common.hpp"
#include "SPF/Telemetry/SCS/Truck.hpp"
#include "SPF/Telemetry/SCS/Trailer.hpp"
#include "SPF/Telemetry/SCS/Job.hpp"
#include "SPF/Telemetry/SCS/Navigation.hpp"
#include "SPF/Telemetry/SCS/Controls.hpp"
#include "SPF/Telemetry/SCS/Events.hpp"
#include "SPF/Telemetry/SCS/Gearbox.hpp"
#include "SPF/Utils/Signal.hpp" // Added for Utils::Signal

SPF_NS_BEGIN

namespace Modules {
/**
 * @class ITelemetryService
 * @brief An abstract interface for providing read-only access to telemetry data.
 *
 * This interface decouples telemetry data consumers (like UI or plugins)
 * from the concrete implementation that provides the data.
 */
class ITelemetryService {
 public:
  virtual ~ITelemetryService() = default;

  // Granular accessors for different data categories
  virtual const SPF::Telemetry::SCS::GameState& GetGameState() const = 0;
  virtual const SPF::Telemetry::SCS::Timestamps& GetTimestamps() const = 0;
  virtual const SPF::Telemetry::SCS::CommonData& GetCommonData() const = 0;
  virtual const SPF::Telemetry::SCS::TruckConstants& GetTruckConstants() const = 0;
  virtual const SPF::Telemetry::SCS::TruckData& GetTruckData() const = 0;
  virtual const std::vector<SPF::Telemetry::SCS::Trailer>& GetTrailers() const = 0;
  virtual const SPF::Telemetry::SCS::JobConstants& GetJobConstants() const = 0;
  virtual const SPF::Telemetry::SCS::JobData& GetJobData() const = 0;
  virtual const SPF::Telemetry::SCS::NavigationData& GetNavigationData() const = 0;
  virtual const SPF::Telemetry::SCS::Controls& GetControls() const = 0;
  virtual const SPF::Telemetry::SCS::SpecialEvents& GetSpecialEvents() const = 0;
  virtual const SPF::Telemetry::SCS::GameplayEvents& GetGameplayEvents() const = 0;
  virtual const SPF::Telemetry::SCS::GearboxConstants& GetGearboxConstants() const = 0;
  virtual const std::string& GetLastGameplayEventId() const = 0;

  // Signal Accessors
  virtual Utils::Signal<void(const SPF::Telemetry::SCS::GameState&)>& GetGameStateSignal() = 0;
  virtual Utils::Signal<void(const SPF::Telemetry::SCS::Timestamps&)>& GetTimestampsSignal() = 0;
  virtual Utils::Signal<void(const SPF::Telemetry::SCS::CommonData&)>& GetCommonDataSignal() = 0;
  virtual Utils::Signal<void(const SPF::Telemetry::SCS::TruckConstants&)>& GetTruckConstantsSignal() = 0;
  virtual Utils::Signal<void(const SPF::Telemetry::SCS::TrailerConstants&)>& GetTrailerConstantsSignal() = 0;
  virtual Utils::Signal<void(const SPF::Telemetry::SCS::TruckData&)>& GetTruckDataSignal() = 0;
  virtual Utils::Signal<void(const std::vector<SPF::Telemetry::SCS::Trailer>&)>& GetTrailersSignal() = 0;
  virtual Utils::Signal<void(const SPF::Telemetry::SCS::JobConstants&)>& GetJobConstantsSignal() = 0;
  virtual Utils::Signal<void(const SPF::Telemetry::SCS::JobData&)>& GetJobDataSignal() = 0;
  virtual Utils::Signal<void(const SPF::Telemetry::SCS::NavigationData&)>& GetNavigationDataSignal() = 0;
  virtual Utils::Signal<void(const SPF::Telemetry::SCS::Controls&)>& GetControlsSignal() = 0;
  virtual Utils::Signal<void(const SPF::Telemetry::SCS::SpecialEvents&)>& GetSpecialEventsSignal() = 0;
  virtual Utils::Signal<void(const char*, const SPF::Telemetry::SCS::GameplayEvents&)>& GetGameplayEventsSignal() = 0;
  virtual Utils::Signal<void(const SPF::Telemetry::SCS::GearboxConstants&)>& GetGearboxConstantsSignal() = 0;

  /**
   * @brief Gets the time elapsed since the last frame.
   * @return Delta time in seconds.
   */
  virtual float GetDeltaTime() const = 0;
};

}  // namespace Modules
SPF_NS_END
