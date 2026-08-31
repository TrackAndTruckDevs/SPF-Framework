#pragma once

namespace SPF::Rendering {

// Enum to represent the graphics API being used by the game.
enum class RenderAPI {
  Unknown,
  D3D11,
  D3D12,
  OpenGL,
};

}  // namespace SPF::Rendering
