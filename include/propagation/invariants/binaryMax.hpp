#pragma once
#include <cmath>

#include "propagation/invariants/invariant.hpp"
#include "propagation/solver.hpp"
#include "types.hpp"

namespace atlantis::propagation {

/**
 * Invariant for output <- max(x, y)
 *
 */
class BinaryMax : public Invariant {
 private:
  VarId _output, _x, _y;

 public:
  explicit BinaryMax(SolverBase& solver, VarId output, VarId x, VarId y);
  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation