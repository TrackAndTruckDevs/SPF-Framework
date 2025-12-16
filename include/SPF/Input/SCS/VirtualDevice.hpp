#pragma once

#include "SPF/Namespace.hpp"
#include "SPF/Telemetry/Sdk.hpp"

#include <string>
#include <vector>
#include <queue>

SPF_NS_BEGIN
namespace Input::SCS {
/**
 * @class VirtualDevice
 * @brief Represents a virtual input device to be registered with the game.
 *
 * This class holds the definition of the device (its name, inputs)
 * and maintains a queue of events to be polled by the game.
 */
class VirtualDevice {
 public:
  VirtualDevice(std::string name, std::string displayName, scs_input_device_type_t type);

  // --- Configuration ---
  void AddButton(std::string name, std::string displayName);
  void AddAxis(std::string name, std::string displayName);

  // --- Event Queue ---
  void PushButtonPress(const std::string& name);
  void PushButtonRelease(const std::string& name);
  void PushAxisChange(const std::string& name, float value);

  // --- SDK Interaction ---
  bool HasPendingEvents() const;
  scs_input_event_t PopEvent();
  scs_input_device_t ToSCSSDKDevice(scs_context_t context, scs_input_event_callback_t event_callback, scs_input_active_callback_t active_callback);

  const std::string& GetName() const { return m_name; }

 private:
  // --- Device Properties ---
  std::string m_name;
  std::string m_displayName;
  scs_input_device_type_t m_type;
  std::vector<scs_input_device_input_t> m_inputs;
  std::vector<std::string> m_inputNames;         // For quick name-to-index lookup
  std::vector<std::string> m_inputDisplayNames;  // To provide stable c_str for display names

  // --- Event State ---
  std::queue<scs_input_event_t> m_eventQueue;
};

}  // namespace Input::SCS
SPF_NS_END
