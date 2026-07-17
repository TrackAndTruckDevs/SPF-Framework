#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/Renderer/ITexture.hpp"
#include "SPF/Renderer/RendererBase.hpp"
#include "SPF/Utils/Signal.hpp"

#include <cstddef>
#include <memory>
#include <windef.h>

SPF_NS_BEGIN

namespace UI {
class UIManager;
}
namespace Logging {
class Logger;
}
namespace Hooks {
class OpenGLHook;
}

namespace Rendering {

class OpenGLRendererImpl : public RendererBase {
 public:
  OpenGLRendererImpl(Renderer& renderer, UI::UIManager& uiManager);
  ~OpenGLRendererImpl() override;

  void Init() override;
  void Shutdown() override;

  std::unique_ptr<ITexture> CreateTextureFromMemory(const unsigned char* data, size_t size) override;
  void RefreshFontAtlas() override;

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
