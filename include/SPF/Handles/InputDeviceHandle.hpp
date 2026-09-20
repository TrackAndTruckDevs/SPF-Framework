#pragma once

#include "SPF/Handles/IHandle.hpp"
#include "SPF/Input/SCS/VirtualDevice.hpp"

#include <string>
#include <utility>

namespace SPF::Handles {
class InputDeviceHandle : public IHandle {
 public:
  Input::SCS::VirtualDevice* const device;
  const std::string ownerName;

  InputDeviceHandle(Input::SCS::VirtualDevice* device, std::string ownerName) : device(device), ownerName(std::move(ownerName)) {}
};
}  // namespace SPF::Handles
