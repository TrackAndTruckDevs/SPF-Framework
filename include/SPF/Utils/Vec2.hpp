#pragma once

namespace SPF::Utils {

/**
 * @brief A simple 2D vector.
 *
 * @tparam T The type of the vector's components.
 */
template <typename T>
struct Vec2 {
  T x;
  T y;

  Vec2() = default;
  Vec2(T x, T y) : x(x), y(y) {}
  explicit Vec2(T value) : x(value), y(value) {}
};

using Vec2f = Vec2<float>;
using Vec2i = Vec2<int>;
using Vec2u = Vec2<unsigned int>;

}  // namespace SPF::Utils
