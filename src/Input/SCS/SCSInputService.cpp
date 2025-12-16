#include "SPF/Input/SCS/SCSInputService.hpp"
#include "SPF/Input/SCS/VirtualDevice.hpp"
#include "SPF/Logging/Logger.hpp"
#include "SPF/Events/EventManager.hpp"
#include "SPF/Input/InputEvents.hpp"
#include <fmt/core.h>

SPF_NS_BEGIN
namespace Input::SCS {
SCSInputService::SCSInputService(Logging::Logger& logger, Events::EventManager& eventManager) : m_logger(logger), m_eventManager(eventManager) {}

SCSInputService::~SCSInputService() = default;

void SCSInputService::Initialize(const scs_input_init_params_t* const params) {
  m_logger.Info("SCSInputService initializing...");
  const auto* versioned_params = static_cast<const scs_input_init_params_v100_t*>(params);
  m_register_device_func = versioned_params->register_device;

  if (!m_register_device_func) {
    m_logger.Error("Failed to get register_device function from SCS SDK.");
    return;
  }
}

void SCSInputService::Shutdown() {
  m_logger.Info("SCSInputService shutting down...");
  m_devices.clear();
}

VirtualDevice* SCSInputService::CreateDevice(const std::string& name, const std::string& displayName, scs_input_device_type_t type) {
  m_devices.push_back(std::make_unique<VirtualDevice>(name, displayName, type));
  return m_devices.back().get();
}

VirtualDevice* SCSInputService::GetDevice(const std::string& name) {
  for (const auto& device : m_devices) {
    if (device->GetName() == name) {
      return device.get();
    }
  }
  return nullptr;
}

void SCSInputService::RegisterCreatedDevices() {
  if (!m_register_device_func) {
    m_logger.Error("Cannot register devices: register_device function is not available.");
    return;
  }

  m_logger.Info("Registering {} created virtual devices...", m_devices.size());

  for (const auto& device : m_devices) {
    scs_input_device_t sdk_device = device->ToSCSSDKDevice(this, &SCSInputService::StaticEventCallback, &SCSInputService::StaticActiveCallback);
    if (m_register_device_func(&sdk_device) == SCS_RESULT_ok) {
      m_logger.Info("  -> Successfully registered device: {}", device->GetName());
    } else {
      m_logger.Error("  -> Failed to register device: {}", device->GetName());
    }
  }
}

scs_result_t SCSAPIFUNC SCSInputService::StaticEventCallback(scs_input_event_t* const event_info, const scs_u32_t flags, const scs_context_t context) {
  if (!context) return SCS_RESULT_generic_error;
  return static_cast<SCSInputService*>(context)->HandleEventPoll(event_info, flags);
}

void SCSAPIFUNC SCSInputService::StaticActiveCallback(const scs_u8_t active, const scs_context_t context) {
  if (!context) return;
  // The context passed by the game is the one we provided during registration, which is `this` SCSInputService.
  // However, the SDK doesn't tell us WHICH device this callback is for if we register multiple.
  // For now, we assume it applies to all our devices or the first one.
  static_cast<SCSInputService*>(context)->HandleActivityChange(active != 0);
}

scs_result_t SCSInputService::HandleEventPoll(scs_input_event_t* const event_info, scs_u32_t flags) {
  // For now, we assume only one device. A more robust implementation would need a way
  // to identify which device the game is polling for.
  if (m_devices.empty()) {
    return SCS_RESULT_not_found;
  }

  auto& device = m_devices.front();
  if (device->HasPendingEvents()) {
    *event_info = device->PopEvent();
    return SCS_RESULT_ok;
  }

  return SCS_RESULT_not_found;
}

void SCSInputService::HandleActivityChange(bool isActive) {
  // As with the event poll, we assume this applies to our first/main device for now.
  if (m_devices.empty()) return;

  const auto& deviceName = m_devices.front()->GetName();
  m_logger.Info("Input device '{}' activity changed to: {}", deviceName, isActive ? "ACTIVE" : "INACTIVE");

  m_eventManager.System.OnInputDeviceActivityChanged.Call({deviceName, isActive});
}

}  // namespace Input::SCS
SPF_NS_END