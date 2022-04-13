#pragma once

#include <cassert>
#include <limits>

#include "constraint.hpp"
#include "core/types.hpp"
#include "variables/intVar.hpp"

class Engine;
class InSparseDomain : public Constraint {
 private:
  const VarId _x;
  const Int _offset;
  std::vector<int> _valueViolation;

 public:
  InSparseDomain(VarId violationId, VarId x,
                 const std::vector<DomainEntry>& domain);

  void registerVars(Engine&) override;
  void updateBounds(Engine&) override;
  void recompute(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
};
