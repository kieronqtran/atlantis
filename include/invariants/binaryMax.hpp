#pragma once
#include <cmath>

#include "core/engine.hpp"
#include "core/types.hpp"
#include "invariants/invariant.hpp"

/**
 * Invariant for output <- max(x, y)
 *
 */
class BinaryMax : public Invariant {
 private:
  const VarId _output, _x, _y;

 public:
  explicit BinaryMax(Engine& engine, VarId output, VarId x, VarId y);
  void registerVars() override;
  void updateBounds(bool widenOnly = false) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  VarId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};