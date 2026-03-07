#pragma once

#include "SPF/Namespace.hpp"
#include <cstdint>

SPF_NS_BEGIN

namespace Rendering {

/**
 * @class ITexture
 * @brief Interface for a graphics texture object.
 */
class ITexture {
public:
    virtual ~ITexture() = default;

    /** @brief Gets the underlying API-specific handle (ID3D11ShaderResourceView*, GLuint, etc.) */
    virtual void* GetHandle() const = 0;

    /** @brief Gets the width of the texture in pixels. */
    virtual uint32_t GetWidth() const = 0;

    /** @brief Gets the height of the texture in pixels. */
    virtual uint32_t GetHeight() const = 0;
};

} // namespace Rendering

SPF_NS_END
