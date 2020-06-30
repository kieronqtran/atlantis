#include "core/intVar.hpp"

#include <limits>

#include "core/types.hpp"

extern Id NULL_ID;
IntVar::IntVar(Int t_lowerBound, Int t_upperBound)
    : IntVar(NULL_ID, t_lowerBound, t_upperBound) {}

IntVar::IntVar(Id t_id, Int t_lowerBound, Int t_upperBound)
    : IntVar(t_id, 0, t_lowerBound, t_upperBound) {}

IntVar::IntVar(Id t_id, Int initValue, Int t_lowerBound, Int t_upperBound)
    : IntVar(NULL_TIMESTAMP, t_id, initValue, t_lowerBound, t_upperBound) {}

IntVar::IntVar(Timestamp t, Id t_id, Int initValue, Int t_lowerBound,
               Int t_upperBound)
    : Var(t_id),
      m_value(t, initValue),  // todo: We need both a time-zero (when
                              // initialisation happens) but also a dummy time.
      m_lowerBound(t_lowerBound),
      m_upperBound(t_upperBound) {
  if (t_lowerBound > t_upperBound) {
    throw std::out_of_range(
        "Lower bound must be smaller than or equal to upper bound");
  }
}
