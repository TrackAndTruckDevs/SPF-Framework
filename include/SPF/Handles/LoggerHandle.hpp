#pragma once

#include "SPF/Handles/IHandle.hpp"

#include <memory>
#include <utility>

namespace SPF::Logging {
class Logger;
}  // namespace SPF::Logging

namespace SPF::Handles {
/**
 * @brief A handle for the Logger API.
 *
 * This handle owns a shared_ptr to a specific logger instance provided
 * by the LoggerFactory.
 */
struct LoggerHandle : IHandle {
  std::shared_ptr<Logging::Logger> logger;

  explicit LoggerHandle(std::shared_ptr<Logging::Logger> logger) : logger(std::move(logger)) {}
};
}  // namespace Handles
