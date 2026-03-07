#pragma once

#include <string>
#include <filesystem>
#include "SPF/Namespace.hpp"

SPF_NS_BEGIN

namespace Utils {

/**
 * @brief Utility class for cryptographic hashing operations.
 */
class HashUtils {
public:
    /**
     * @brief Calculates the MD5 hash of a file.
     * @param path Path to the file.
     * @return Hex-encoded MD5 hash string, or an empty string on failure.
     */
    static std::string CalculateFileMD5(const std::filesystem::path& path);
};

} // namespace Utils

SPF_NS_END
