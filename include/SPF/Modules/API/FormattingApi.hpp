#pragma once

#include "SPF/Namespace.hpp"

#include "SPF/SPF_API/SPF_Formatting_API.h"

#include <cstddef>

SPF_NS_BEGIN
namespace Modules::API {

class FormattingApi {
 public:
  /**
   * @brief Fills the provided SPF_Formatting_API struct with pointers to the C-style trampoline functions.
   * @param api The struct to fill.
   */
  static void FillFormattingApi(SPF_Formatting_API* api);

 private:
  // --- C-API Trampoline Implementations ---

  static int Fmt_Format(char* buffer, size_t buffer_size, const char* format, ...);
};

}  // namespace Modules::API
SPF_NS_END
