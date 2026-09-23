/**
 * @file ExampleVirtInputAPI.cpp
 * @brief Complete example of the SPF VirtInput API + KeyBinds axis test — input injection.
 *
 * @details Two tabs:
 *   1. Virtual Input — creates a virtual device in OnLoad and injects button/axis
 *      events from the UI (hold-to-honk, throttle slider).
 *   2. Input Test — reads the analog 'Test.Axis' keybind action and visualizes it.
 *
 * INIT ORDER (VirtInputAPI_OnLoad):
 *   1. Virt_CreateDevice
 *   2. Virt_AddButton / Virt_AddAxis  — layout is fixed at registration time
 *   3. Virt_Register  — registering with zero inputs is useless
 *
 * RENDERING never creates devices — it only sends one-shot events on the existing handle.
 * Virtual devices MUST be created during OnLoad (see SPF_VirtInput_API.h notes).
 */
#include "ExampleVirtInputAPI.hpp"

#include "SPF/SPF_API/SPF_KeyBinds_API.h"
#include "SPF/SPF_API/SPF_Logger_API.h"
#include "SPF/SPF_API/SPF_UI_API.h"
#include "SPF/SPF_API/SPF_VirtInput_API.h"

#include "ExamplePlugin.hpp"

namespace ExamplePlugin {

// =====================================================================================
// Lifecycle — create + register the virtual device once at load
// =====================================================================================

void VirtInputAPI_OnLoad() {
  auto input = g_ctx.loadAPI ? g_ctx.loadAPI->input : nullptr;
  if (!input) {
    return;
  }
  auto logger_api = g_ctx.loadAPI->logger;
  auto logger = logger_api ? logger_api->Log_GetContext(PLUGIN_NAME) : nullptr;

  // Create: pluginName scopes ownership, deviceName is the stable key,
  // displayName is what the game UI shows, type selects the button/axis layout family.
  g_ctx.virtualDevice = input->Virt_CreateDevice(PLUGIN_NAME, "Example_virtual_device", "ExamplePlugin Virtual Controller", SPF_INPUT_DEVICE_TYPE_GENERIC);
  if (!g_ctx.virtualDevice) {
    if (logger_api && logger) {
      logger_api->Log(logger, SPF_LOG_ERROR, "Failed to create virtual device.");
    }
    return;
  }

  // Add inputs BEFORE register — the layout is fixed once registered.
  // inputName is the programmatic key used later for Press/Release/SetAxisValue;
  // displayName is the human-readable label in the game's binding UI.
  input->Virt_AddButton(g_ctx.virtualDevice, "virt_honk", "Virtual Honk");
  input->Virt_AddAxis(g_ctx.virtualDevice, "virt_throttle", "Virtual Throttle");

  // Register: publishes the finished layout to the framework.
  if (input->Virt_Register(g_ctx.virtualDevice)) {
    if (logger_api && logger) {
      logger_api->Log(logger, SPF_LOG_INFO, "Successfully registered virtual device.");
    }
  } else if (logger_api && logger) {
    logger_api->Log(logger, SPF_LOG_ERROR, "Failed to register virtual device.");
  }
}

void VirtInputAPI_OnUnload() {
  // Devices registered mid-session cannot be re-registered after load
  // (Virt_Register fails on re-entry) — just drop the handle.
  g_ctx.virtualDevice = nullptr;
}

// =====================================================================================
// Virtual Input tab — inject events that the game consumes through its binding menu
// =====================================================================================

void RenderVirtInputTab(SPF_UI_API* ui, void* user_data) {
  (void)user_data;

  if (!g_ctx.coreAPI || !g_ctx.coreAPI->input || !g_ctx.virtualDevice || !ui) {
    ui->UI_Text("Virtual Input API not available or device not initialized.");
    return;
  }
  ui->UI_Text("Use the controls below to simulate input.");
  ui->UI_Text("You must bind 'Virtual Honk' and 'Virtual Throttle' in the game's keybinding menu.");
  ui->UI_Separator();

  // Example of a virtual button: hold the ImGui button → keep the virtual button pressed.
  ui->UI_Text("Virtual Honk Button:");
  ui->UI_Button("Hold to Honk", 0, 0);  // The button itself is just for show.
  if (ui->UI_IsItemActive()) {
    // While the ImGui button is held down, press the virtual button.
    g_ctx.coreAPI->input->Virt_PressButton(g_ctx.virtualDevice, "virt_honk");
  } else {
    // When the ImGui button is released, release the virtual button.
    g_ctx.coreAPI->input->Virt_ReleaseButton(g_ctx.virtualDevice, "virt_honk");
  }
  ui->UI_Separator();

  // Example of a virtual axis: continuous value the game reads as a trigger/stick.
  static float throttle_value = 0.0f;
  ui->UI_Text("Virtual Throttle Axis:");
  if (ui->UI_SliderFloat("Throttle", &throttle_value, 0.0f, 1.0f, "%.2f", SPF_SLIDER_FLAG_NONE)) {
    // When the slider value changes, update the virtual axis value.
    g_ctx.coreAPI->input->Virt_SetAxisValue(g_ctx.virtualDevice, "virt_throttle", throttle_value);
  }
}

// =====================================================================================
// Input Test tab — poll KeyBinds 'Test.Axis' and show binding details
// =====================================================================================

void RenderInputTestTab(SPF_UI_API* ui, void* user_data) {
  (void)user_data;

  if (!g_ctx.coreAPI || !g_ctx.coreAPI->keybinds || !g_ctx.keybindsHandle || !ui) {
    ui->UI_Text("Keybinds API not available.");
    return;
  }

  ui->UI_Text("Use this tab to test analog axis bindings and view detailed binding info.");
  ui->UI_Text("Assign any axis to 'Test.Axis' in settings.");
  ui->UI_Separator();

  const char* actionName = "Test.Axis";
  auto keybinds = g_ctx.coreAPI->keybinds;
  auto format = g_ctx.coreAPI->formatting;

  // Axis actions accumulate device input into a [-1,1] (or [0,1]) value each frame.
  float val = keybinds->Kbind_GetActionValue(g_ctx.keybindsHandle, actionName);

  char val_buf[64];
  format->Fmt_Format(val_buf, sizeof(val_buf), "Raw Action Value: %.4f", val);
  ui->UI_Text(val_buf);

  // Visualize 0.0 to 1.0 (e.g. Triggers)
  ui->UI_Text("Unipolar (0..1):");
  float uni_fraction = (val < 0.0f) ? 0.0f : val;
  ui->UI_ProgressBar(uni_fraction, -1, 0, "");

  ui->UI_Spacing();

  // Visualize -1.0 to 1.0 (e.g. Sticks)
  ui->UI_Text("Bipolar (-1..1):");
  float bi_fraction = (val + 1.0f) / 2.0f;
  ui->UI_ProgressBar(bi_fraction, -1, 0, "");

  ui->UI_Separator();
  ui->UI_Text("Active Bindings Information:");

  int bindingCount = keybinds->Kbind_GetBindingCount(g_ctx.keybindsHandle, actionName);
  if (bindingCount == 0) {
    ui->UI_TextColored(1.0f, 0.5f, 0.5f, 1.0f, "No bindings assigned to this action.");
    // Kbind_OpenRebindPopup(..., -1) opens the exact same "press a key" modal the native
    // Settings window uses, so this custom tab can offer key assignment with full parity
    // (and sync) with the framework's own UI.
    if (ui->UI_Button("Assign a key...", 0, 0)) {
      keybinds->Kbind_OpenRebindPopup(g_ctx.keybindsHandle, actionName, -1);
    }
  } else {
    for (int i = 0; i < bindingCount; ++i) {
      char line_buf[256];
      char name_buf[128];
      // Kbind_GetBindingDisplayName includes the same device icon (keyboard/gamepad/mouse
      // glyph) the native Settings UI shows, so this tree node label matches it visually.
      keybinds->Kbind_GetBindingDisplayName(g_ctx.keybindsHandle, actionName, i, name_buf, sizeof(name_buf));

      format->Fmt_Format(line_buf, sizeof(line_buf), "[Binding %d] Name: %s", i + 1, name_buf);
      if (ui->UI_TreeNode(line_buf)) {
        if (ui->UI_Button("Rebind...", 0, 0)) {
          keybinds->Kbind_OpenRebindPopup(g_ctx.keybindsHandle, actionName, i);
        }
        ui->UI_SameLine(0, 8);
        // Kbind_OpenBindingDetailsPopup opens the same "gear icon" popup the native
        // Settings window uses for behavior/press-threshold/axis tuning.
        if (ui->UI_Button("Details...", 0, 0)) {
          keybinds->Kbind_OpenBindingDetailsPopup(g_ctx.keybindsHandle, actionName, i);
        }

        // 1. Type
        SPF_BindingType type = keybinds->Kbind_GetBindingType(g_ctx.keybindsHandle, actionName, i);
        const char* typeStr = (type == SPF_BINDING_KEYBOARD)        ? "Keyboard"
                              : (type == SPF_BINDING_GAMEPAD)       ? "Gamepad Button"
                              : (type == SPF_BINDING_MOUSE)         ? "Mouse Button"
                              : (type == SPF_BINDING_JOYSTICK)      ? "Joystick Button"
                              : (type == SPF_BINDING_CHORD)         ? "Chord"
                              : (type == SPF_BINDING_GAMEPAD_AXIS)  ? "Gamepad Axis"
                              : (type == SPF_BINDING_MOUSE_AXIS)    ? "Mouse Axis"
                              : (type == SPF_BINDING_JOYSTICK_AXIS) ? "Joystick Axis"
                                                                    : "Unknown";
        format->Fmt_Format(line_buf, sizeof(line_buf), "Type: %s", typeStr);
        ui->UI_BulletText(line_buf);

        // 2. Behavior
        SPF_ActivationBehavior behavior = keybinds->Kbind_GetBindingBehavior(g_ctx.keybindsHandle, actionName, i);
        const char* behaviorStr = (behavior == SPF_BEHAVIOR_HOLD) ? "Hold" : (behavior == SPF_BEHAVIOR_TOGGLE) ? "Toggle" : "N/A";
        format->Fmt_Format(line_buf, sizeof(line_buf), "Behavior: %s", behaviorStr);
        ui->UI_BulletText(line_buf);

        // 3. Press Type
        SPF_PressType press = keybinds->Kbind_GetBindingPressType(g_ctx.keybindsHandle, actionName, i);
        const char* pressStr = (press == SPF_PRESS_SHORT) ? "Short" : (press == SPF_PRESS_LONG) ? "Long" : "N/A";
        format->Fmt_Format(line_buf, sizeof(line_buf), "Press Type: %s", pressStr);
        ui->UI_BulletText(line_buf);

        // 4. Mode
        SPF_InputMode mode = keybinds->Kbind_GetBindingMode(g_ctx.keybindsHandle, actionName, i);
        const char* modeStr = (mode == SPF_MODE_ANALOG) ? "Analog" : (mode == SPF_MODE_DIGITAL) ? "Digital" : "N/A";
        format->Fmt_Format(line_buf, sizeof(line_buf), "Mode: %s", modeStr);
        ui->UI_BulletText(line_buf);

        // 5. Side
        SPF_AxisSide side = keybinds->Kbind_GetBindingSide(g_ctx.keybindsHandle, actionName, i);
        const char* sideStr = (side == SPF_SIDE_POSITIVE) ? "Positive" : (side == SPF_SIDE_NEGATIVE) ? "Negative" : (side == SPF_SIDE_BOTH) ? "Both" : "N/A";
        format->Fmt_Format(line_buf, sizeof(line_buf), "Side: %s", sideStr);
        ui->UI_BulletText(line_buf);

        // 6. Accumulator
        SPF_AccumulatorMode acc = keybinds->Kbind_GetBindingAccumulatorMode(g_ctx.keybindsHandle, actionName, i);
        const char* accStr = (acc == SPF_ACCUMULATOR_ON) ? "ON" : (acc == SPF_ACCUMULATOR_OFF) ? "OFF" : "N/A";
        format->Fmt_Format(line_buf, sizeof(line_buf), "Accumulator: %s", accStr);
        ui->UI_BulletText(line_buf);

        ui->UI_TreePop();
      }
    }
  }
}

}  // namespace ExamplePlugin
