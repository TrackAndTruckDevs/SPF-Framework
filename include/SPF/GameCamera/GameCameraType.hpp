#pragma once

#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace GameCamera {
// Internal, type-safe enum for camera types using standardized names
enum class GameCameraType {
  DeveloperFreeCamera = 0,
  BehindCamera = 1,
  InteriorCamera = 2,
  BumperCamera = 3,
  WindowCamera = 4,
  CabinCamera = 5,
  WheelCamera = 6,
  TopCamera = 7,
  TVCamera = 9,
  Unknown = -1,
};
}  // namespace GameCamera
SPF_NS_END
