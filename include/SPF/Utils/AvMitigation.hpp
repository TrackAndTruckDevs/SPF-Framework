#pragma once

#include "SPF/Namespace.hpp"
SPF_NS_BEGIN
namespace Utils {
/**
 * @brief Function to reduce the probability of false positives from antiviruses.
 * Adds calls to standard Windows APIs and references a block of text to change the file's entropy.
 */
void InitializeAvMitigation();
}  // namespace Utils
SPF_NS_END
