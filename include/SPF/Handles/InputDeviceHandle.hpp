#pragma once

#include "SPF/Handles/IHandle.hpp"

SPF_NS_BEGIN

namespace Input::SCS {
class VirtualDevice;
}  // namespace Input::SCS

namespace Handles {
class InputDeviceHandle : public IHandle {
 public:
  Input::SCS::VirtualDevice* const device;
  const std::string ownerName;

  InputDeviceHandle(Input::SCS::VirtualDevice* device, std::string ownerName) 
      : device(device), ownerName(std::move(ownerName)) {}
};
}  // namespace Handles
SPF_NS_END
