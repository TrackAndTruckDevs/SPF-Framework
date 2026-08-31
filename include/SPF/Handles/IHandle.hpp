#pragma once

namespace SPF::Handles {
/**
 * @brief Base interface for all API handles.
 *
 * This empty struct with a virtual destructor allows HandleManager
 * to manage the lifetime of different handle types polymorphically.
 */
struct IHandle {
  virtual ~IHandle() = default;
};
}  // namespace SPF::Handles
