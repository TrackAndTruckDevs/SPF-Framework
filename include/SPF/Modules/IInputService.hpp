#pragma once

#include "SPF/Namespace.hpp"
#include "SPF/Telemetry/Sdk.hpp"  // Include all SDK headers
#include <string>                 // Added for std::string

// Forward declaration for the SDK init params
struct scs_input_init_params_t;

SPF_NS_BEGIN

namespace Input::SCS {
class VirtualDevice;
}  // namespace Input::SCS

namespace Modules {
/**
 * @class IInputService
 * @brief An abstract interface for a service that registers and manages
 * virtual input devices with the game.
 */
class IInputService {
 public:
  virtual ~IInputService() = default;

  /**
   * @brief Initializes the input service.
   * @param params The SDK-provided initialization parameters.
   */
  virtual void Initialize(const scs_input_init_params_t* const params) = 0;

  /**
   * @brief Shuts down the input service.
   */
  virtual void Shutdown() = 0;

  virtual Input::SCS::VirtualDevice* CreateDevice(const std::string& name, const std::string& displayName, scs_input_device_type_t type) = 0;

  /**
   * @brief Finds a registered virtual device by its unique name.
   * @param name The name of the device to find.
   * @return A pointer to the device, or nullptr if not found.
   */
  virtual Input::SCS::VirtualDevice* GetDevice(const std::string& name) = 0;

  /**
   * @brief Iterates over all created devices and registers them with the game SDK.
   * This should only be called from within the scs_input_init context.
   */
  virtual void RegisterCreatedDevices() = 0;
};

}  // namespace Modules
SPF_NS_END
