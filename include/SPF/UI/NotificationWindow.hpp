#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/SPF_API/SPF_UI_API.h"  // For SPF_Notification_DisplayMode
#include "SPF/UI/BaseWindow.hpp"

#include "imgui.h"
#include "nlohmann/json_fwd.hpp"

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>


SPF_NS_BEGIN

namespace UI {

/**
 * @brief Framework-owned window for displaying temporary notifications.
 * @details Supports multiple display modes:
 *          - TOP: Single notification at the top-center (newer replaces older).
 *          - STACK: Multiple notifications stacked from the bottom-right upwards.
 *          - STICKY: Sticky notification at call position, until clicked outside.
 */
class NotificationWindow : public BaseWindow {
 public:
  NotificationWindow(const std::string& componentName, const std::string& windowId);
  virtual ~NotificationWindow() = default;

  /**
   * @brief Triggers a new notification.
   */
  void Show(const std::string& message, int type, float duration, SPF_Notification_DisplayMode mode);

  /**
   * @brief Extended version of Show with full parameter control.
   */
  SPF_Notification_Handle ShowEx(const SPF_Notification_Params& params);

  /**
   * @brief Programmatically closes a notification.
   */
  void Hide(SPF_Notification_Handle handle);

  void RenderContent() override;
  ImGuiWindowFlags GetExtraWindowFlags() const override;

  /**
   * @brief STICKY popups need interaction.
   */
  bool IsInteractive() const override;
  bool IsPersistent() const final { return false; }

  void ApplySettings(const nlohmann::ordered_json& settings) override {}
  nlohmann::ordered_json GetCurrentSettings() const override { return nlohmann::ordered_json::object(); }

 private:
  struct NotificationData {
    std::string message;
    int type = 0;
    SPF_Notification_DisplayMode mode = SPF_NOTIF_MODE_TOP;
    std::chrono::steady_clock::time_point startTime;
    float duration = 3.0f;
    float currentYOffset = 0.0f;  // Used for smooth stacking animations
    ImVec2 popupPos;              // Used for STICKY mode
    bool isClosing = false;

    // Extended parameters
    uint64_t handle = 0;
    bool isProgrammatic = false;
    bool initialized = false;
    ImVec4 customColor = ImVec4(0, 0, 0, 0);
    std::string customIcon;
  };

  std::vector<NotificationData> m_notifications;
  uint64_t m_nextHandle = 1;

  // Internal styling helpers
  void GetTypeStyle(const NotificationData& notif, const char** out_icon, ImVec4& out_color);
  void RenderSingleNotification(NotificationData& notif, int index);
};

}  // namespace UI

SPF_NS_END
