#pragma once

#include "SPF/Namespace.hpp"

#include <csetjmp>
#include <errhandlingapi.h>
#include <excpt.h>
#include <minwindef.h>
#include <winnt.h>

SPF_NS_BEGIN
namespace Utils {

#ifdef _MSC_VER
// ---------- MSVC: native __try/__except (catches both SEH and C++ with /EHa) ----------
template <typename Fn>
inline bool InvokeSafe(Fn&& fn, DWORD* outCode = nullptr) {
  __try {
    fn();
    return true;
  } __except (EXCEPTION_EXECUTE_HANDLER) {
    if (outCode) *outCode = GetExceptionCode();
    return false;
  }
}
#else
// ---------- MinGW/GCC: Vectored Exception Handler + try/catch ----------
namespace detail {

struct GuardState {
  jmp_buf buffer;
  bool active = false;
  DWORD code = 0;
};

inline GuardState& GetState() {
  static thread_local GuardState state;
  return state;
}

inline LONG CALLBACK VectoredHandler(PEXCEPTION_POINTERS ep) {
  auto& state = GetState();
  if (state.active) {
    // Let C++ exceptions (0xE06D7363) propagate to try/catch for proper stack unwinding
    if (ep->ExceptionRecord->ExceptionCode == 0xE06D7363) {
      return EXCEPTION_CONTINUE_SEARCH;
    }
    state.code = ep->ExceptionRecord->ExceptionCode;
    state.active = false;
    longjmp(state.buffer, 1);
  }
  return EXCEPTION_CONTINUE_SEARCH;
}

inline bool EnsureHandlerInstalled() {
  static bool installed = false;
  if (!installed) {
    AddVectoredExceptionHandler(1, VectoredHandler);
    installed = true;
  }
  return installed;
}
}  // namespace detail

template <typename Fn>
inline bool InvokeSafe(Fn&& fn, DWORD* outCode = nullptr) {
  detail::EnsureHandlerInstalled();
  auto& state = detail::GetState();

  if (setjmp(state.buffer) == 0) {
    state.active = true;
    try {
      fn();
    } catch (...) {
      state.active = false;
      if (outCode) *outCode = 0xE06D7363;
      return false;
    }
    state.active = false;
    return true;
  } else {
    if (outCode) *outCode = state.code;
    return false;
  }
}
#endif

}  // namespace Utils
SPF_NS_END
