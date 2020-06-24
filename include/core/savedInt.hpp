#pragma once

#include <stdexcept>

#include "core/tracer.hpp"
#include "types.hpp"

class SavedInt {
 private:
  Timestamp m_tmpTime;
  Int m_savedValue;
  Int m_tmpValue;

 public:
  SavedInt(Timestamp initTime, const Int& initValue)
      : m_tmpTime(initTime),
        m_savedValue(initValue), 
        m_tmpValue(initValue) {}

  // inline Int getResetValue(Timestamp currentTime) noexcept {
  //   return currentTime == m_tmpTime ? m_tmpValue : (m_tmpValue =
  //   m_savedValue);
  // }

  [[gnu::always_inline]] inline Timestamp getTmpTimestamp() const {
    return m_tmpTime;
  }

  [[gnu::always_inline]] inline Int getValue(Timestamp currentTime) const
      noexcept {
    return currentTime == m_tmpTime ? m_tmpValue : m_savedValue;
  }

  [[gnu::always_inline]] inline Int getCommittedValue() const noexcept {
    return m_savedValue;
  }

  [[gnu::always_inline]] inline void setValue(Timestamp currentTime,
                                              const Int& value) noexcept {
    m_tmpTime = currentTime;
    m_tmpValue = value;
  }

  [[gnu::always_inline]] inline void incValue(Timestamp currentTime,
                                              const Int& inc) noexcept {
    m_tmpValue = (currentTime == m_tmpTime ? m_tmpValue : m_savedValue) + inc;
    m_tmpTime = currentTime;
  }
  [[gnu::always_inline]] inline void commitValue(Int value) noexcept {
    m_savedValue = value;
    // m_tmpValue = value;  // If we do not update the tmp value, then it is not
                         // clear what the correct value is at tmp_time.
  }

  [[gnu::always_inline]] inline void commit() noexcept {
    // assert(false);  // todo: do we really want this? Very dangerous to just
                    // commit regardless of time.
    m_savedValue = m_tmpValue;
  }

  [[gnu::always_inline]] inline void commitIf(
      Timestamp currentTime) noexcept {
    if (m_tmpTime == currentTime) {
      m_savedValue = m_tmpValue;
    }
  }
};
