#ifndef VALUE_WITH_DELTA_H
#define VALUE_WITH_DELTA_H

#include <algorithm>
#include <cmath>

template <class T> class ValueWithDelta {
public:
  ValueWithDelta() = default;
  ValueWithDelta(const T value, const T delta)
      : m_value(value), m_delta(delta) {}

  bool operator==(const ValueWithDelta &other) const {
    return std::abs(m_value - other.m_value) < std::max(m_delta, other.m_delta);
  }

private:
  T m_value{0.0};
  T m_delta{0.0};
};

#endif // VALUE_WITH_DELTA_H
