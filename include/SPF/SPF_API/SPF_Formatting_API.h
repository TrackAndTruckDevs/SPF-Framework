#pragma once

/**
 * @file SPF_Formatting_API.h
 * @brief C-API for safe string formatting.
 * @details This API provides a safe way for plugins to format strings using
 *          printf-style arguments without the risks of passing variadic
 *          arguments across DLL boundaries.
 */

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct SPF_Formatting_API
 * @brief API for safe, cross-DLL string formatting.
 *
 * @section Workflow
 * 1.  Create a character buffer in your plugin's code.
 * 2.  Call the `Format` function, passing your buffer, its size, and a
 *     printf-style format string with its arguments.
 * 3.  The framework will safely format the string into your buffer.
 * 4.  Use the formatted string in your buffer with other APIs like the
 *     Logger or UI.
 *
 * @section Example
 * @code{.cpp}
 * const SPF_Formatting_API* fmt_api = ...; // Obtained from the framework
 * const SPF_Logger_API* logger_api = ...;
 *
 * char my_buffer[256];
 * int value = 42;
 * fmt_api->Format(my_buffer, sizeof(my_buffer), "The value is %d", value);
 *
 * logger_api->Log(my_logger, SPF_LOG_INFO, my_buffer);
 * @endcode
 */
typedef struct SPF_Formatting_API {
    /**
     * @brief Formats a string using printf-style arguments and stores it in a buffer.
     *
     * @details This function is a safe wrapper around `vsnprintf`. It takes a format
     *          string and a variable number of arguments, formats them, and writes
     *          the result into the provided buffer, ensuring not to overflow it.
     *
     * @param[out] buffer A pointer to the character buffer to write the result into.
     * @param buffer_size The total size of the `buffer`.
     * @param format The printf-style format string.
     * @param ... The variable arguments corresponding to the format string.
     * @return The number of characters that would have been written if the buffer
     *         was large enough (excluding the null terminator), or a negative
     *         value if an error occurred. This is the same behavior as `vsnprintf`.
     */
    int (*Format)(char* buffer, size_t buffer_size, const char* format, ...);

} SPF_Formatting_API;


#ifdef __cplusplus
}
#endif
