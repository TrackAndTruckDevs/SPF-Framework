#pragma once

#include "SPF/Modules/IInputService.hpp"
#include "SPF/Telemetry/Sdk.hpp"

#include <memory>
#include <vector>
#include <string>

SPF_NS_BEGIN

// Forward declarations
namespace Logging {
class Logger;
}
namespace Events {
class EventManager;
}
namespace Input::SCS {
class VirtualDevice;
}

namespace Input::SCS {
/**
 * @class SCSInputService
 * @brief Manages virtual input devices and communicates with the SCS Input SDK.
 */
class SCSInputService final : public Modules::IInputService {
 public:
  SCSInputService(Logging::Logger& logger, Events::EventManager& eventManager);
  ~SCSInputService() override;

  // --- IInputService Implementation ---
  void Initialize(const scs_input_init_params_t* const params) override;
  void Shutdown() override;
  VirtualDevice* GetDevice(const std::string& name) override;

  // --- Device Management ---
  VirtualDevice* CreateDevice(const std::string& name, const std::string& displayName, scs_input_device_type_t type) override;
  void RegisterCreatedDevices() override;

 private:
  // --- Static Callbacks for SCS SDK ---
  static scs_result_t SCSAPIFUNC StaticEventCallback(scs_input_event_t* const event_info, const scs_u32_t flags, const scs_context_t context);
  static void SCSAPIFUNC StaticActiveCallback(const scs_u8_t active, const scs_context_t context);

  // --- Internal Event Handlers ---
  scs_result_t HandleEventPoll(scs_input_event_t* const event_info, scs_u32_t flags);
  void HandleActivityChange(bool isActive);

  Logging::Logger& m_logger;
  Events::EventManager& m_eventManager;

  // --- SDK State ---
  scs_input_register_device_t m_register_device_func = nullptr;

  // --- Owned Devices ---
  std::vector<std::unique_ptr<VirtualDevice>> m_devices;
};

}  // namespace Input::SCS
SPF_NS_END
