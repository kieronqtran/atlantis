#pragma once

#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/propagation/violationInvariants/violationInvariant.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

class BoolLessThan : public ViolationInvariant {
 private:
  VarViewId _x, _y;

 public:
  explicit BoolLessThan(SolverBase&, VarId violationId, VarViewId x,
                        VarViewId y);

  explicit BoolLessThan(SolverBase&, VarViewId violationId, VarViewId x,
                        VarViewId y);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarViewId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

}  // namespace atlantis::propagation
