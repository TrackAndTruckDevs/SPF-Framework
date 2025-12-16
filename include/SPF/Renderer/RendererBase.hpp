#pragma once

#include "SPF/Namespace.hpp"

SPF_NS_BEGIN

namespace Rendering {

class Renderer;

/**
 * @class RendererBase
 * @brief An abstract base class defining the interface for a renderer implementation.
 *
 * This "contract" ensures that any specific renderer implementation (D3D11, OpenGL, etc.)
 * provides a common set of methods, allowing the main Renderer class to work with them
 * polymorphically.
 */
class RendererBase {
 public:
  /**
   * @brief Constructs the base renderer.
   * @param renderer The main Renderer instance.
   */
  explicit RendererBase(Renderer& renderer) : m_renderer(renderer) {}

  // Virtual destructor is mandatory for a base class with virtual methods.
  virtual ~RendererBase() = default;

  RendererBase(const RendererBase&) = delete;
  RendererBase& operator=(const RendererBase&) = delete;

  /**
   * @brief Initializes the renderer implementation.
   */
  virtual void Init() = 0;

  /** @brief Shuts down the renderer implementation. */
  virtual void Shutdown() = 0;

 protected:
  Renderer& m_renderer;
};

}  // namespace Rendering

SPF_NS_END
