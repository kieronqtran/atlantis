#pragma once

#include <vector>

#include "atlantis/propagation/invariants/invariant.hpp"
#include "atlantis/propagation/solverBase.hpp"
#include "atlantis/propagation/types.hpp"
#include "atlantis/propagation/variables/committableInt.hpp"
#include "atlantis/types.hpp"

namespace atlantis::propagation {

class GlobalCardinalityOpen : public Invariant {
 private:
  std::vector<VarId> _outputs;
  std::vector<VarViewId> _inputs;
  std::vector<Int> _cover;
  std::vector<Int> _coverVarIndex;
  std::vector<CommittableInt> _counts;
  Int _offset;
  void increaseCount(Timestamp ts, Int value);
  void decreaseCountAndUpdateOutput(Timestamp ts, Int value);
  void increaseCountAndUpdateOutput(Timestamp ts, Int value);

 public:
  GlobalCardinalityOpen(SolverBase&, std::vector<VarId>&& outputs,
                        std::vector<VarViewId>&& inputs,
                        std::vector<Int>&& cover);

  GlobalCardinalityOpen(SolverBase&, std::vector<VarViewId>&& outputs,
                        std::vector<VarViewId>&& inputs,
                        std::vector<Int>&& cover);

  void registerVars() override;
  void updateBounds(bool widenOnly) override;
  void close(Timestamp) override;
  void recompute(Timestamp) override;
  void notifyInputChanged(Timestamp, LocalId) override;
  void commit(Timestamp) override;
  VarViewId nextInput(Timestamp) override;
  void notifyCurrentInputChanged(Timestamp) override;
};

inline void GlobalCardinalityOpen::increaseCount(Timestamp ts, Int value) {
  if (0 <= value - _offset &&
      value - _offset < static_cast<Int>(_coverVarIndex.size()) &&
      _coverVarIndex[value - _offset] >= 0) {
    _counts[_coverVarIndex[value - _offset]].incValue(ts, 1);
  }
}

inline void GlobalCardinalityOpen::decreaseCountAndUpdateOutput(Timestamp ts,
                                                                Int value) {
  if (0 <= value - _offset &&
      value - _offset < static_cast<Int>(_coverVarIndex.size()) &&
      _coverVarIndex[value - _offset] >= 0) {
    updateValue(ts, _outputs[_coverVarIndex[value - _offset]],
                _counts[_coverVarIndex[value - _offset]].incValue(ts, -1));
  }
}

inline void GlobalCardinalityOpen::increaseCountAndUpdateOutput(Timestamp ts,
                                                                Int value) {
  if (0 <= value - _offset &&
      value - _offset < static_cast<Int>(_coverVarIndex.size()) &&
      _coverVarIndex[value - _offset] >= 0) {
    updateValue(ts, _outputs[_coverVarIndex[value - _offset]],
                _counts[_coverVarIndex[value - _offset]].incValue(ts, 1));
  }
}

}  // namespace atlantis::propagation
