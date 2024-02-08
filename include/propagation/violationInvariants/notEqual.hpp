#pragma once

#include <algorithm>
#include <functional>

#include "propagation/solver.hpp"
#include "propagation/variables/intVar.hpp"
#include "propagation/violationInvariants/violationInvariant.hpp"
#include "types.hpp"

namespace atlantis::propagation {

class NotEqual : public ViolationInvariant {
 private:
  VarId _x, _y;

 public:
  explicit NotEqual(SolverBase&, VarId violationId, VarId x, VarId y);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation