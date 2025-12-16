#pragma once

#include <memory>
#include <windows.h>
#include <SPF/Renderer/RendererBase.hpp>
#include "SPF/Utils/Signal.hpp"

SPF_NS_BEGIN

namespace UI { class UIManager; }
namespace Logging { class Logger; }
namespace Hooks { class OpenGLHook; }

namespace Rendering {

class OpenGLRendererImpl : public RendererBase {
 public:
  OpenGLRendererImpl(Renderer& renderer, UI::UIManager& uiManager);
  ~OpenGLRendererImpl() override;

  void Init() override;
  void Shutdown() override;

 private:
  void OnInit(HDC hdc);
  void OnPresent(HDC hdc);

  UI::UIManager& m_uiManager;
  std::shared_ptr<Logging::Logger> m_logger;

  // --- OpenGL specific resources ---
  HDC m_hdc = nullptr;
  HGLRC m_originalContext = nullptr;
  bool m_isImGuiInitialized = false;

  // --- Sinks for Hook Signals ---
  Utils::Sink<void(HDC hdc)> m_onInitSink;
  Utils::Sink<void(HDC hdc)> m_onPresentSink;
};

}  // namespace Rendering

SPF_NS_END
