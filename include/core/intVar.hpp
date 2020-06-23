#pragma once

#include "core/savedInt.hpp"
#include "core/tracer.hpp"
#include "core/types.hpp"
#include "core/var.hpp"

class Engine;  // Forward declaration

class IntVar : public Var {
 private:
  SavedInt m_value;

  [[gnu::always_inline]] inline void setValue(const Timestamp& timestamp,
                                              Int value) {
#ifdef VERBOSE_TRACE
#include <iostream>
    std::cout << "IntVar(" << m_id << ").setValue(" << timestamp << "," << value
              << ")"
              << "\n";
#endif
    m_value.setValue(timestamp, value);
  }
  [[gnu::always_inline]] inline void incValue(const Timestamp& timestamp,
                                              Int inc) {
#ifdef VERBOSE_TRACE
#include <iostream>
    std::cout << "IntVar(" << m_id << ").incValue(" << timestamp << "," << inc
              << ")"
              << "\n";
#endif
    m_value.incValue(timestamp, inc);
  }

  [[gnu::always_inline]] inline void commit() { m_value.commit(); }
  [[gnu::always_inline]] inline void commitValue(Int value) {
#ifdef VERBOSE_TRACE
#include <iostream>
    std::cout << "IntVar(" << m_id << ").commitValue(" << value << ")"
              << "\n";
#endif
    m_value.commitValue(value);
  }
  [[gnu::always_inline]] inline void commitIf(const Timestamp& timestamp) {
    m_value.commitIf(timestamp);
  }

  friend class Engine;

 public:
  IntVar();
  IntVar(Id t_id);
  IntVar(Id t_id, Int initValue);
  IntVar(Id t_id, Int initValue, Int lowerBound, Int upperBound);
  ~IntVar() = default;

  [[gnu::always_inline]] inline bool hasChanged(const Timestamp& t) const {
    return m_value.getValue(t) != m_value.getCommittedValue();
  }
  [[gnu::always_inline]] inline const Timestamp& getTmpTimestamp() const {
    return m_value.getTmpTimestamp();
  }
  [[gnu::always_inline]] inline Int getValue(const Timestamp& t) const {
    return m_value.getValue(t);
  }
  [[gnu::always_inline]] inline Int getCommittedValue() const {
    return m_value.getCommittedValue();
  }
  [[gnu::always_inline]] inline bool inDomain(Int t_value) const {
    return m_value.inDomain(t_value);
  }
  [[gnu::always_inline]] inline void updateDomain(Int lowerBound, Int upperBound) {
    m_value.updateDomain(lowerBound, upperBound);
  }
};
