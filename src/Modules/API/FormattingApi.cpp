#include "SPF/Modules/API/FormattingApi.hpp"
#include <cstdarg>
#include <cstdio>

SPF_NS_BEGIN
namespace Modules::API {

int FormattingApi::F_Format(char* buffer, size_t buffer_size, const char* format, ...) {
    if (!buffer || buffer_size == 0 || !format) {
        return -1;
    }

    va_list args;
    va_start(args, format);
    int result = vsnprintf(buffer, buffer_size, format, args);
    va_end(args);

    return result;
}

void FormattingApi::FillFormattingApi(SPF_Formatting_API* api) {
    if (!api) return;

    api->Format = &FormattingApi::F_Format;
}

} // namespace Modules::API
SPF_NS_END
