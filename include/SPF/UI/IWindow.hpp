#pragma once

#include "SPF/Namespace.hpp"
#include "SPF/Events/SystemEvents.hpp"
#include "SPF/SPF_API/SPF_Plugin.h"
#include "SPF/SPF_API/SPF_UI_API.h"

#include <string>
#include <nlohmann/json.hpp>

SPF_NS_BEGIN
namespace UI {
class IWindow {
 public:
  virtual ~IWindow() = default;

  // Called by UIManager to render the window
  virtual void Render() = 0;

  // --- State & Identity ---
  virtual const std::string& GetWindowId() const = 0;
  virtual const std::string& GetComponentName() const = 0;
  virtual bool IsVisible() const = 0;
  virtual bool IsInteractive() const = 0;
  virtual bool IsFocused() const = 0;

  virtual void Focus() = 0;

  virtual const char* GetWindowTitle() const = 0;

  // --- Configuration ---

  /**
   * @brief Applies settings from a json object to the window.
   * @param settings The json object containing the window's settings.
   */
  virtual void ApplySettings(const nlohmann::json& settings) = 0;
  virtual void SetDrawCallback(SPF_DrawCallback callback) = 0;

  /**
   * @brief Gets the current state of the window as a json object.
   * @return A json object representing the window's current settings.
   */
  virtual nlohmann::json GetCurrentSettings() const = 0;

  // --- Event Handlers (Optional) ---
  virtual void OnUpdateCheckSucceeded(const Events::System::OnUpdateCheckSucceeded& e) {}
  virtual void OnUpdateCheckFailed(const Events::System::OnUpdateCheckFailed& e) {}
  virtual void OnPatronsFetchCompleted(const Events::System::OnPatronsFetchCompleted& e) {}
};
}  // namespace UI
SPF_NS_END
