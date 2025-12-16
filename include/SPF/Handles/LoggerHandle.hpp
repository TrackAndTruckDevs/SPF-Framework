#pragma once

#include "SPF/Handles/IHandle.hpp"
#include "SPF/Namespace.hpp"

#include <memory>

SPF_NS_BEGIN

namespace Logging {
class Logger;
}  // namespace Logging

namespace Handles {
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

SPF_NS_END
