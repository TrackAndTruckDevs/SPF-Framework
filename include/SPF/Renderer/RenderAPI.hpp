#pragma once

#include "SPF/Namespace.hpp"

SPF_NS_BEGIN

namespace Rendering {

// Enum to represent the graphics API being used by the game.
enum class RenderAPI {
  Unknown,
  D3D11,
  D3D12,
  OpenGL,
};

}  // namespace Rendering

SPF_NS_END
