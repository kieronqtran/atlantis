#pragma once

#include <memory>
#include <vector>

#include "../core/constraint.hpp"
#include "../core/engine.hpp"
#include "../core/intVar.hpp"
#include "../core/tracer.hpp"
#include "../core/types.hpp"

class Equal : public Constraint {
 private:
  VarId m_x;
  VarId m_y;

 public:
  Equal(VarId violationId, VarId x, VarId y);
  
  ~Equal() = default;
  virtual void init(Timestamp, Engine&) override;
  virtual void recompute(Timestamp, Engine&) override;
  virtual void notifyIntChanged(Timestamp t, Engine& e, LocalId id, Int newValue) override;
  virtual void commit(Timestamp, Engine&) override;
  virtual VarId getNextDependency(Timestamp, Engine&) override;
  virtual void notifyCurrentDependencyChanged(Timestamp, Engine& e) override;
};
