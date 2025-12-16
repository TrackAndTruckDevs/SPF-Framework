#pragma once

#include <vector>
#include <algorithm>

#include "SPF/Namespace.hpp"

#include "Delegate.hpp"

SPF_NS_BEGIN

namespace Utils {

template <typename>
class Signal;

template <typename>
class Sink;

/**
 * @brief Signal handler used to notify multiple delegates.
 *        Based on ry-core project by Piotr Krupa (https://github.com/Hary309/hry-core).
 *
 * @tparam Return Return type of a delegate function
 * @tparam Args Types of arguments of a delegate function
 */
template <typename Return, typename... Args>
class Signal<Return(Args...)> final {
  friend Sink<Return(Args...)>;

 private:
  using Delegate_t = Delegate<Return(Args...)>;

 private:
  std::vector<Delegate_t> m_calls;

 public:
  void Call(Args... args) noexcept {
    // Create a copy of the delegates to call, in case a delegate modifies the list during iteration.
    auto callsCopy = m_calls;
    for (Delegate_t& delegate : callsCopy) {
      delegate.Call(std::forward<Args>(args)...);
    }
  }

 private:
  void Add(Delegate_t delegate) noexcept { m_calls.push_back(delegate); }

  void Remove(Delegate_t delegate) noexcept {
    auto it = std::find_if(m_calls.begin(), m_calls.end(), [&delegate](const Delegate_t& a) { return a == delegate; });

    if (it != m_calls.end()) {
      m_calls.erase(it);
    }
  }
};

/**
 * @brief A Sink is an adapter to a Signal class.
 *        In the destructor, all delegates connected to the sink are disconnected.
 *
 * @tparam Return Return type of a delegate function
 * @tparam Args Types of arguments of a delegate function
 */
template <typename Return, typename... Args>
class Sink<Return(Args...)> final {
 private:
  using Signal_t = Signal<Return(Args...)>;
  using Delegate_t = typename Signal_t::Delegate_t;

 private:
  Signal_t* m_signal;
  std::vector<Delegate_t> m_internalCalls;

 public:
  Sink(Signal_t& signal) noexcept : m_signal(&signal) {}
  Sink(Sink&&) noexcept = default;
  Sink(const Sink&) noexcept = default;
  Sink& operator=(Sink&&) noexcept = default;
  Sink& operator=(const Sink&) noexcept = default;

  ~Sink() noexcept {
    for (auto& delegate : m_internalCalls) {
      m_signal->Remove(delegate);
    }
  }

  template <auto FuncAddr>
  void Connect() noexcept {
    Delegate_t delegate;
    delegate.template Connect<FuncAddr>();

    m_signal->Add(delegate);
    m_internalCalls.push_back(delegate);
  }

  template <auto MethodAddr, typename T>
  void Connect(T* content) noexcept {
    Delegate_t delegate;
    delegate.template Connect<MethodAddr>(content);

    m_signal->Add(delegate);
    m_internalCalls.push_back(delegate);
  }

  void Clear() {
    for (auto& delegate : m_internalCalls) {
      m_signal->Remove(delegate);
    }

    m_internalCalls.clear();
  }
};

template <typename Return, typename... Args>
Sink(Signal<Return(Args...)>&) -> Sink<Return(Args...)>;

}  // namespace Utils

SPF_NS_END
