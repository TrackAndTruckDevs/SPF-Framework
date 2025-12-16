#pragma once

#include <functional>
#include <type_traits>

#include "SPF/Namespace.hpp"

SPF_NS_BEGIN

namespace Utils {

template <typename>
class Delegate;

template <auto>
struct ConnectArg {};

template <auto Addr>
inline static ConnectArg<Addr> ConnectArg_v;

/**
 * @brief Utility class to send information around the framework.
 *        Used for the events system. Based on ry-core project by Piotr Krupa (https://github.com/Hary309/hry-core)
 *
 * @tparam Return Return type of a delegate function
 * @tparam Args Types of arguments of a delegate function
 */
template <typename Return, typename... Args>
class Delegate<Return(Args...)> final {
 public:
  using Function_t = Return(void*, Args...);

 private:
  Function_t* m_function = nullptr;
  void* m_context = nullptr;

 public:
  Delegate() = default;

  template <auto FuncAddr>
  Delegate(ConnectArg<FuncAddr> /*unused*/) noexcept {
    Connect<FuncAddr>();
  }

  template <auto CtxFuncAddr, typename T>
  Delegate(ConnectArg<CtxFuncAddr> /*unused*/, T* context) noexcept {
    Connect<CtxFuncAddr>(context);
  }

  Delegate(Function_t* func, void* context) noexcept { Connect(func, context); }

  template <auto FuncAddr>
  void Connect() noexcept {
    static_assert(std::is_invocable_r_v<Return, decltype(FuncAddr), Args...>, "Passed function doesn't meet declared function template");

    m_function = [](void* /*unused*/, Args... args) -> Return { return static_cast<Return>(std::invoke(FuncAddr, std::forward<Args>(args)...)); };

    m_context = nullptr;
  }

  template <auto CtxFuncAddr, typename T>
  void Connect(T* context) noexcept {
    static_assert(std::is_invocable_r_v<Return, decltype(CtxFuncAddr), T*, Args...>, "Passed method doesn't meet declared method template");

    m_function = [](void* context, Args... args) -> Return { return static_cast<Return>(std::invoke(CtxFuncAddr, static_cast<T*>(context), std::forward<Args>(args)...)); };

    m_context = context;
  }

  void Connect(Function_t* func, void* context) noexcept {
    m_function = func;
    m_context = context;
  }

  Return Call(Args... args) const {
    if (m_function) {
      return m_function(m_context, std::forward<Args>(args)...);
    }
  }

  void Reset() noexcept {
    m_function = nullptr;
    m_context = nullptr;
  }

  Return operator()(Args... args) const { return Call(std::forward<Args>(args)...); }

  bool operator==(const Delegate<Return(Args...)>& other) const noexcept { return (m_function == other.m_function && m_context == other.m_context); }
};

template <typename Return, typename... Args>
auto FunctionPtr(Return (*)(Args...)) -> Return (*)(Args...);

template <typename Class, typename Return, typename... Args, typename... Other>
auto FunctionPtr(Return (Class::*)(Args...), Other&&...) -> Return (*)(Args...);

template <typename Class, typename Return, typename... Args, typename... Other>
auto FunctionPtr(Return (Class::*)(Args...) const, Other&&...) -> Return (*)(Args...);

template <typename... Types>
using FunctionPtr_t = decltype(FunctionPtr(std::declval<Types>()...));

template <auto FuncAddr>
Delegate(ConnectArg<FuncAddr>) -> Delegate<std::remove_pointer_t<FunctionPtr_t<decltype(FuncAddr)>>>;

template <auto CtxFuncAddr, typename T>
Delegate(ConnectArg<CtxFuncAddr>, T* context) -> Delegate<std::remove_pointer_t<FunctionPtr_t<decltype(CtxFuncAddr), T>>>;

template <auto FuncAddr>
auto Dlg() noexcept {
  return Delegate<std::remove_pointer_t<FunctionPtr_t<decltype(FuncAddr)>>>{ConnectArg_v<FuncAddr>};
}

template <auto CtxFuncAddr, typename T>
auto Dlg(T* content) noexcept {
  return Delegate<std::remove_pointer_t<FunctionPtr_t<decltype(CtxFuncAddr), T>>>{ConnectArg_v<CtxFuncAddr>, content};
}

}  // namespace Utils

SPF_NS_END
