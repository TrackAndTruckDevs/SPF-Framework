#pragma once

#include "SPF/Namespace.hpp"

#include <atomic>
#include <csetjmp>
#include <errhandlingapi.h>
#include <excpt.h>
#include <minwindef.h>
#include <mutex>
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

namespace detail {
// No process-wide handler is installed on MSVC; nothing to remove on teardown.
inline void RemoveHandler() {}
}  // namespace detail
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

inline PVOID& HandlerHandle() {
  static PVOID handle = nullptr;
  return handle;
}

// Once the handler is removed (full shutdown, DLL about to unload) it must
// never come back: lazy installers like logging would otherwise resurrect a
// process-wide pointer into freed memory. Reset()/sdk reinit never touches
// this — the latch lives exactly as long as this DLL instance.
inline std::atomic<bool>& HandlerRemoved() {
  static std::atomic<bool> removed{false};
  return removed;
}

inline bool EnsureHandlerInstalled() {
  if (HandlerRemoved().load(std::memory_order_acquire)) {
    return false;
  }
  // Logging starts on every thread early; call_once keeps the install single.
  static std::once_flag installOnce;
  std::call_once(installOnce, [] { HandlerHandle() = AddVectoredExceptionHandler(1, VectoredHandler); });
  return HandlerHandle() != nullptr;
}

// Must run before the DLL unloads: the OS keeps the raw function pointer in a
// process-wide handler list, so an unloaded module would crash the game on the
// next first-chance exception anywhere in the process.
inline void RemoveHandler() {
  HandlerRemoved().store(true, std::memory_order_release);
  if (PVOID handle = HandlerHandle()) {
    RemoveVectoredExceptionHandler(handle);
    HandlerHandle() = nullptr;
  }
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
