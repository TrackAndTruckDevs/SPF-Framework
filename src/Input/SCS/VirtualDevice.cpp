#include "SPF/Input/SCS/VirtualDevice.hpp"
#include <stdexcept>
#include <algorithm>
#include <cstring>  // For strncpy

SPF_NS_BEGIN
namespace Input::SCS {
// Helper to find an input index by name.
// Returns -1 if not found.
int find_input_index(const std::vector<std::string>& names, const std::string& name) {
  auto it = std::find(names.begin(), names.end(), name);
  if (it != names.end()) {
    return static_cast<int>(std::distance(names.begin(), it));
  }
  return -1;
}

VirtualDevice::VirtualDevice(std::string name, std::string displayName, scs_input_device_type_t type)
    : m_name(std::move(name)), m_displayName(std::move(displayName)), m_type(type) {
  // Reserve some space to avoid frequent reallocations
  m_inputs.reserve(32);
  m_inputNames.reserve(32);
  m_inputDisplayNames.reserve(32);
}

void VirtualDevice::AddButton(std::string name, std::string displayName) {
  if (find_input_index(m_inputNames, name) != -1) {
    // Or log a warning
    return;
  }
  m_inputNames.push_back(name);
  m_inputDisplayNames.push_back(displayName);
  m_inputs.push_back({nullptr, nullptr, SCS_VALUE_TYPE_bool});
}

void VirtualDevice::AddAxis(std::string name, std::string displayName) {
  if (find_input_index(m_inputNames, name) != -1) {
    return;
  }
  m_inputNames.push_back(name);
  m_inputDisplayNames.push_back(displayName);
  m_inputs.push_back({nullptr, nullptr, SCS_VALUE_TYPE_float});
}

void VirtualDevice::PushButtonPress(const std::string& name) {
  int index = find_input_index(m_inputNames, name);
  if (index == -1) return;  // Or throw

  scs_input_event_t evt{};
  evt.input_index = index;
  evt.value_bool.value = true;
  m_eventQueue.push(evt);
}

void VirtualDevice::PushButtonRelease(const std::string& name) {
  int index = find_input_index(m_inputNames, name);
  if (index == -1) return;  // Or throw

  scs_input_event_t evt{};
  evt.input_index = index;
  evt.value_bool.value = false;
  m_eventQueue.push(evt);
}

void VirtualDevice::PushAxisChange(const std::string& name, float value) {
  int index = find_input_index(m_inputNames, name);
  if (index == -1) return;  // Or throw

  scs_input_event_t evt{};
  evt.input_index = index;
  evt.value_float.value = value;
  m_eventQueue.push(evt);
}

bool VirtualDevice::HasPendingEvents() const { return !m_eventQueue.empty(); }

scs_input_event_t VirtualDevice::PopEvent() {
  if (m_eventQueue.empty()) {
    throw std::runtime_error("Popping from an empty event queue");
  }
  scs_input_event_t evt = m_eventQueue.front();
  m_eventQueue.pop();
  return evt;
}

scs_input_device_t VirtualDevice::ToSCSSDKDevice(scs_context_t context, scs_input_event_callback_t event_callback, scs_input_active_callback_t active_callback) {
  // The SCS SDK expects the string pointers inside the scs_input_device_input_t array
  // to be valid for the lifetime of the registration. We stored nullptr initially and now
  // update them to point to the stable strings in our member vectors.
  for (size_t i = 0; i < m_inputs.size(); ++i) {
    m_inputs[i].name = m_inputNames[i].c_str();
    m_inputs[i].display_name = m_inputDisplayNames[i].c_str();
  }

  scs_input_device_t device_info{};
  device_info.name = m_name.c_str();
  device_info.display_name = m_displayName.c_str();
  device_info.type = m_type;
  device_info.input_count = static_cast<scs_u32_t>(m_inputs.size());
  device_info.inputs = m_inputs.data();
  device_info.callback_context = context;
  device_info.input_active_callback = active_callback;
  device_info.input_event_callback = event_callback;

  return device_info;
}

}  // namespace Input::SCS
SPF_NS_END
