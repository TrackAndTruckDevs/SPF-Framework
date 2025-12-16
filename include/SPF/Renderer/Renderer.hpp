#pragma once

#include <memory>
#include <chrono>

#include "SPF/Namespace.hpp"
#include "SPF/Renderer/RenderAPI.hpp"

SPF_NS_BEGIN

// Forward declarations of types from other modules
namespace Core {
class Core;
}
namespace Events {
class EventManager;
}
namespace Logging {
class Logger;
}
namespace UI {
class UIManager;
}
// Forward declarations of types from this module (Rendering)
namespace Rendering {
class RendererBase;
class D3D11RendererImpl;
class D3D12RendererImpl;
class OpenGLRendererImpl;

/**
 * @class Renderer
 * @brief High-level abstraction over the specific render API implementation.
 *
 * This class determines which graphics API the game is using, creates the
 * corresponding implementation (e.g., D3D11RendererImpl), and delegates
 * all rendering work to it. It serves as the bridge between low-level
 * rendering events and the high-level Core logic.
 */
class Renderer {
 public:
  /**
   * @brief Constructs the Renderer.
   * @param core The main Core instance.
   * @param eventManager The main EventManager instance.
   */
  Renderer(Core::Core& core, Events::EventManager& eventManager, UI::UIManager& uiManager);
  ~Renderer();

  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;

  /**
   * @brief Initializes the renderer by creating and initializing the specific implementation
   *        based on the detected API.
   */
  void Init();

  /**
   * @brief Gets the graphics API that was detected during construction.
   * @return The detected RenderAPI enum value.
   */
  RenderAPI GetDetectedAPI() const;

  // --- Callbacks from Implementation ---

  /**
   * @brief A callback invoked by the renderer implementation once it's fully initialized.
   * This signals to the Core that UI-dependent components can now be initialized.
   */
  void OnRendererInit();

  /**
   * @brief A callback invoked by the renderer implementation each frame to perform UI rendering.
   */
  void OnRendererRenderImGui();

 private:
  /**
   * @brief Performs a lightweight check to determine the most likely graphics API used by the game.
   * @return The detected RenderAPI.
   */
  RenderAPI DetectRenderAPI();

  Core::Core& m_core;
  UI::UIManager& m_uiManager;
  std::unique_ptr<RendererBase> m_impl;
  std::shared_ptr<Logging::Logger> m_logger;
  RenderAPI m_detectedAPI;
  std::chrono::steady_clock::time_point m_lastFrameTime;
};

}  // namespace Rendering

SPF_NS_END
