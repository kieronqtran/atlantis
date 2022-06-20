#pragma once

#include <algorithm>
#include <limits>
#include <vector>

#include "core/engine.hpp"
#include "core/types.hpp"
#include "invariants/invariant.hpp"

/**
 * Invariant for output <- varArray[index] where varArray is a vector of VarId.
 * NOTE: the index set is 1 based (first element is varArray[1], not
 * varArray[0])
 *
 */

class Element2dVar : public Invariant {
 private:
  const std::vector<std::vector<VarId>> _varMatrix;
  const std::array<const VarId, 2> _indices;
  const std::array<const Int, 2> _dimensions;
  const std::array<const Int, 2> _offsets;
  const VarId _output;

  [[nodiscard]] inline size_t safeIndex(Int index, size_t pos) const noexcept {
    return std::max<Int>(0,
                         std::min(_dimensions[pos] - 1, index - _offsets[pos]));
  }

  [[nodiscard]] inline size_t safeIndex1(Int index) const noexcept {
    return safeIndex(index, 0);
  }

  [[nodiscard]] inline size_t safeIndex2(Int index) const noexcept {
    return safeIndex(index, 1);
  }

 public:
  explicit Element2dVar(VarId output, VarId index1, VarId index2,
                        std::vector<std::vector<VarId>> varMatrix,
                        Int offset1 = 1, Int offset2 = 1);
  void registerVars(Engine&) override;
  void updateBounds(Engine&, bool widenOnly = false) override;
  void recompute(Timestamp, Engine&) override;
  void notifyInputChanged(Timestamp, Engine&, LocalId) override;
  void commit(Timestamp, Engine&) override;
  VarId nextInput(Timestamp, Engine&) override;
  void notifyCurrentInputChanged(Timestamp, Engine&) override;
};