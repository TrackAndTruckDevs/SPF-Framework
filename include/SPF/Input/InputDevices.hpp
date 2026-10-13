#pragma once

#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace SPF::Input {

/**
 * @brief Device slot of an input that isn't tied to one physical device.
 *
 * A binding with this slot is triggered by any device, and an event with it
 * comes from a source that can't tell its devices apart (keyboard, mouse,
 * XInput).
 */
constexpr uint8_t kAnyDevice = 0;

/**
 * Hardware codes are laid out as 0xTTAADDII: device type, axis flag (0x01),
 * device slot and input index. The slot is kAnyDevice unless the input is tied
 * to one device, so codes without a device keep their old values.
 */
constexpr uint32_t kDeviceSlotMask = 0x0000FF00u;

constexpr uint32_t WithDevice(uint32_t hardwareCode, uint8_t device) { return (hardwareCode & ~kDeviceSlotMask) | (static_cast<uint32_t>(device) << 8); }

constexpr uint8_t DeviceOf(uint32_t hardwareCode) { return static_cast<uint8_t>((hardwareCode & kDeviceSlotMask) >> 8); }

constexpr uint32_t WithAnyDevice(uint32_t hardwareCode) { return hardwareCode & ~kDeviceSlotMask; }

/**
 * @brief Maps stable device identifiers (as saved in a binding's "device"
 * field) to the slots used in hardware codes.
 *
 * Slots are handed out on first sight, from a binding being loaded or from the
 * device sending input, so they're only valid for the session: only the
 * identifier is ever saved.
 */
class InputDevices {
 public:
  /**
   * @brief Gets the slot for a device identifier, assigning one if it's new.
   * @return kAnyDevice for an empty identifier or once all 255 slots are taken.
   */
  static uint8_t SlotFor(const std::string& id) {
    if (id.empty()) return kAnyDevice;
    auto& registry = Instance();
    std::lock_guard<std::mutex> lock(registry.m_mutex);
    auto it = registry.m_slots.find(id);
    if (it != registry.m_slots.end()) return it->second;
    if (registry.m_ids.size() >= 255) return kAnyDevice;
    registry.m_ids.push_back(id);
    uint8_t slot = static_cast<uint8_t>(registry.m_ids.size());
    registry.m_slots.emplace(id, slot);
    return slot;
  }

  /**
   * @brief Gets the identifier of a slot, empty for kAnyDevice.
   */
  static std::string IdOf(uint8_t slot) {
    if (slot == kAnyDevice) return {};
    auto& registry = Instance();
    std::lock_guard<std::mutex> lock(registry.m_mutex);
    return slot <= registry.m_ids.size() ? registry.m_ids[slot - 1] : std::string{};
  }

 private:
  static InputDevices& Instance() {
    static InputDevices instance;
    return instance;
  }

  std::mutex m_mutex;
  std::map<std::string, uint8_t> m_slots;
  std::vector<std::string> m_ids;
};

}  // namespace SPF::Input
