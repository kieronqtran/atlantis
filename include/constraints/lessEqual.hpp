#pragma once

#include "constraint.hpp"
#include "core/types.hpp"
#include "variables/intVar.hpp"

class Engine;

class LessEqual : public Constraint {
 private:
  const VarId _x;
  const VarId _y;

 public:
  LessEqual(VarId violationId, VarId x, VarId y);

  void registerVars(Engine&) override;
  void updateBounds(Engine&, bool widenOnly = false) override;
  void recompute(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
};
