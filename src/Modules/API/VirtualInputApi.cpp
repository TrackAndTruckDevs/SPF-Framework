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

SPF_VirtualDevice_Handle* VirtualInputApi::Virt_CreateDevice(const char* pluginName, const char* deviceName, const char* displayName, SPF_InputDeviceType type) {
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

    auto handle = std::make_unique<Handles::InputDeviceHandle>(device, pluginName);
    return reinterpret_cast<SPF_VirtualDevice_Handle*>(pm.GetHandleManager()->RegisterHandle(pluginName, std::move(handle)));
}

void VirtualInputApi::Virt_AddButton(SPF_VirtualDevice_Handle* h, const char* inputName, const char* displayName) {
    auto* devHandle = reinterpret_cast<Handles::InputDeviceHandle*>(h);
    if (devHandle && devHandle->device && inputName && displayName) {
        devHandle->device->AddButton(inputName, displayName);
    }
}

void VirtualInputApi::Virt_AddAxis(SPF_VirtualDevice_Handle* h, const char* inputName, const char* displayName) {
    auto* devHandle = reinterpret_cast<Handles::InputDeviceHandle*>(h);
    if (devHandle && devHandle->device && inputName && displayName) {
        devHandle->device->AddAxis(inputName, displayName);
    }
}

bool VirtualInputApi::Virt_Register(SPF_VirtualDevice_Handle* h) {
    auto* devHandle = reinterpret_cast<Handles::InputDeviceHandle*>(h);
    if (!devHandle || !devHandle->device) return false;

    auto& pm = PluginManager::GetInstance();
    if (!pm.GetInputService()) return false;

    return pm.GetInputService()->RegisterDevice(devHandle->device, devHandle->ownerName);
}

void VirtualInputApi::Virt_PressButton(SPF_VirtualDevice_Handle* h, const char* inputName) {
    auto* devHandle = reinterpret_cast<Handles::InputDeviceHandle*>(h);
    if (devHandle && devHandle->device && inputName) {
        devHandle->device->PushButtonPress(inputName);
    }
}

void VirtualInputApi::Virt_ReleaseButton(SPF_VirtualDevice_Handle* h, const char* inputName) {
    auto* devHandle = reinterpret_cast<Handles::InputDeviceHandle*>(h);
    if (devHandle && devHandle->device && inputName) {
        devHandle->device->PushButtonRelease(inputName);
    }
}

void VirtualInputApi::Virt_SetAxisValue(SPF_VirtualDevice_Handle* h, const char* inputName, float value) {
    auto* devHandle = reinterpret_cast<Handles::InputDeviceHandle*>(h);
    if (devHandle && devHandle->device && inputName) {
        devHandle->device->PushAxisChange(inputName, value);
    }
}

void VirtualInputApi::FillVirtualInputApi(SPF_VirtInput_API* api) {
    if (!api) return;

    api->Virt_CreateDevice = &VirtualInputApi::Virt_CreateDevice;
    api->Virt_AddButton = &VirtualInputApi::Virt_AddButton;
    api->Virt_AddAxis = &VirtualInputApi::Virt_AddAxis;
    api->Virt_Register = &VirtualInputApi::Virt_Register;
    api->Virt_PressButton = &VirtualInputApi::Virt_PressButton;
    api->Virt_ReleaseButton = &VirtualInputApi::Virt_ReleaseButton;
    api->Virt_SetAxisValue = &VirtualInputApi::Virt_SetAxisValue;
}

} // namespace Modules::API
SPF_NS_END