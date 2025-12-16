#include "SPF/Modules/API/VirtualInputApi.hpp"
#include "SPF/Modules/PluginManager.hpp"
#include "SPF/Handles/InputDeviceHandle.hpp"
#include "SPF/Modules/IInputService.hpp"
#include "SPF/Modules/HandleManager.hpp"
#include "SPF/Input/SCS/VirtualDevice.hpp"
#include "SPF/Logging/LoggerFactory.hpp"

#include <string>
#include <cctype>

SPF_NS_BEGIN
namespace Modules::API {

SPF_VirtualDevice_Handle* VirtualInputApi::I_CreateDevice(const char* pluginName, const char* deviceName, const char* displayName, SPF_InputDeviceType type) {
    auto& pm = PluginManager::GetInstance();
    if (!pluginName || !deviceName || !displayName || !pm.GetInputService() || !pm.GetHandleManager()) return nullptr;

    std::string prefixedName = std::string(pluginName) + "_" + deviceName;

    // Sanitize the name to comply with SDK rules (lowercase, digits, underscore)
    for (char& c : prefixedName) {
        c = std::tolower(static_cast<unsigned char>(c));
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) {
            c = '_';
        }
    }

    auto* device = pm.GetInputService()->CreateDevice(prefixedName, displayName, static_cast<scs_input_device_type_t>(type));
    if (!device) return nullptr;

    auto handle = std::make_unique<Handles::InputDeviceHandle>(device);
    return reinterpret_cast<SPF_VirtualDevice_Handle*>(pm.GetHandleManager()->RegisterHandle(pluginName, std::move(handle)));
}

void VirtualInputApi::I_AddButton(SPF_VirtualDevice_Handle* handle, const char* inputName, const char* displayName) {
    auto* devHandle = reinterpret_cast<Handles::InputDeviceHandle*>(handle);
    if (devHandle && devHandle->device && inputName && displayName) {
        devHandle->device->AddButton(inputName, displayName);
    }
}

void VirtualInputApi::I_AddAxis(SPF_VirtualDevice_Handle* handle, const char* inputName, const char* displayName) {
    auto* devHandle = reinterpret_cast<Handles::InputDeviceHandle*>(handle);
    if (devHandle && devHandle->device && inputName && displayName) {
        devHandle->device->AddAxis(inputName, displayName);
    }
}

bool VirtualInputApi::I_Register(SPF_VirtualDevice_Handle* handle) {
    // This is a slightly more complex case. The device is already created in the SCSInputService.
    // The SCSInputService is responsible for registering it with the game at the correct time (in its Initialize method).
    // This function could perhaps trigger that registration if we change the logic, but for now, we can make it a no-op
    // or have it log that registration is automatic.
    auto& pm = PluginManager::GetInstance();
    auto logger = Logging::LoggerFactory::GetInstance().GetLogger("PluginManager");
    if (logger) logger->Info("I_Register is a no-op. Devices are registered centrally after all plugins are loaded.");
    return true;
}

void VirtualInputApi::I_PressButton(SPF_VirtualDevice_Handle* handle, const char* inputName) {
    auto* devHandle = reinterpret_cast<Handles::InputDeviceHandle*>(handle);
    if (devHandle && devHandle->device && inputName) {
        devHandle->device->PushButtonPress(inputName);
    }
}

void VirtualInputApi::I_ReleaseButton(SPF_VirtualDevice_Handle* handle, const char* inputName) {
    auto* devHandle = reinterpret_cast<Handles::InputDeviceHandle*>(handle);
    if (devHandle && devHandle->device && inputName) {
        devHandle->device->PushButtonRelease(inputName);
    }
}

void VirtualInputApi::I_SetAxisValue(SPF_VirtualDevice_Handle* handle, const char* inputName, float value) {
    auto* devHandle = reinterpret_cast<Handles::InputDeviceHandle*>(handle);
    if (devHandle && devHandle->device && inputName) {
        devHandle->device->PushAxisChange(inputName, value);
    }
}

void VirtualInputApi::FillVirtualInputApi(SPF_Input_API* api) {
    if (!api) return;

    api->CreateDevice = &VirtualInputApi::I_CreateDevice;
    api->AddButton = &VirtualInputApi::I_AddButton;
    api->AddAxis = &VirtualInputApi::I_AddAxis;
    api->Register = &VirtualInputApi::I_Register;
    api->PressButton = &VirtualInputApi::I_PressButton;
    api->ReleaseButton = &VirtualInputApi::I_ReleaseButton;
    api->SetAxisValue = &VirtualInputApi::I_SetAxisValue;
}

} // namespace Modules::API
SPF_NS_END
