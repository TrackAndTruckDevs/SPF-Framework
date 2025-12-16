#pragma once
#include <string>
#include <vector>
#include "SPF/Namespace.hpp"

SPF_NS_BEGIN
namespace Core {
struct InitializationReport {
  struct Issue {
    // Human-readable message for logging
    std::string Message;

    // JSON Pointer path to the problematic key for ConfigService.
    // Can be empty if the issue is not config-related.
    std::string ConfigKeyPath;
  };

  // Service name, MUST match the componentId in FrameworkManifest.hpp
  std::string ServiceName;

  std::vector<std::string> InfoMessages;
  std::vector<Issue> Warnings;
  std::vector<Issue> Errors;

  bool HasIssues() const { return !Warnings.empty() || !Errors.empty(); }
};
}  // namespace Core
SPF_NS_END