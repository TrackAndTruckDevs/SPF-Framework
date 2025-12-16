#pragma once

#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Handles {
/**
 * @brief Base interface for all API handles.
 *
 * This empty struct with a virtual destructor allows HandleManager
 * to manage the lifetime of different handle types polymorphically.
 */
struct IHandle {
  virtual ~IHandle() = default;
};
}  // namespace Handles
SPF_NS_END
