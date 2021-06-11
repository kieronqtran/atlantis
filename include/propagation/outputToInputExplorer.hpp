#pragma once

#include <unordered_set>
#include <vector>

#include "core/types.hpp"
#include "exceptions/exceptions.hpp"
#include "utils/idMap.hpp"

// Forward declare PropagationEngine
class PropagationEngine;

class OutputToInputExplorer {
  PropagationEngine& m_engine;

  std::vector<VarId> variableStack_;
  size_t varStackIdx_ = 0;
  std::vector<InvariantId> invariantStack_;
  size_t invariantStackIdx_ = 0;
  IdMap<VarId, Timestamp> varStableAt;  // last timestamp when a VarID was
                                        // stable (i.e., will not change)
  IdMap<InvariantId, Timestamp> invariantStableAt;
  IdMap<InvariantId, bool> invariantIsOnStack;

  IdMap<VarIdBase, std::unordered_set<VarIdBase>> m_decisionVarAncestor;

  template <bool OutputToInputMarking>
  void preprocessVarStack(Timestamp);

  template <bool OutputToInputMarking>
  bool isUpToDate(VarIdBase);

  void pushVariableStack(VarId v);
  void popVariableStack();
  VarId peekVariableStack();
  void pushInvariantStack(InvariantId);
  void popInvariantStack();
  InvariantId peekInvariantStack();
  void markStable(Timestamp t, VarId v);
  bool isStable(Timestamp t, VarId v);
  bool isStable(Timestamp t, InvariantId v);

  // We expand an invariant by pushing it and its first input variable onto
  // each stack.
  template <bool OutputToInputMarking>
  void expandInvariant(InvariantId inv);

  void notifyCurrentInvariant();

  template <bool OutputToInputMarking>
  bool pushNextInputVariable();

 public:
  OutputToInputExplorer() = delete;
  OutputToInputExplorer(PropagationEngine& e, size_t expectedSize);

  void populateAncestors();
  void registerVar(VarId);
  void registerInvariant(InvariantId);
  /**
   * Register than we want to compute the value of v at time t
   */
  void registerForPropagation(Timestamp, VarId);

  void clearRegisteredVariables();

  template <bool OutputToInputMarking>
  void propagate(Timestamp currentTime);
};

inline void OutputToInputExplorer::registerForPropagation(Timestamp, VarId id) {
  pushVariableStack(id);
}

inline void OutputToInputExplorer::clearRegisteredVariables() {
  varStackIdx_ = 0;
}

inline void OutputToInputExplorer::pushVariableStack(VarId v) {
  variableStack_[varStackIdx_++] = v;
}
inline void OutputToInputExplorer::popVariableStack() { --varStackIdx_; }
inline VarId OutputToInputExplorer::peekVariableStack() {
  return variableStack_[varStackIdx_ - 1];
}

inline void OutputToInputExplorer::pushInvariantStack(InvariantId i) {
  if (invariantIsOnStack.get(i)) {
    throw DynamicCycleException();
  }
  invariantIsOnStack.set(i, true);
  invariantStack_[invariantStackIdx_++] = i;
}
inline void OutputToInputExplorer::popInvariantStack() {
  invariantIsOnStack.set(invariantStack_[--invariantStackIdx_], false);
}

inline InvariantId OutputToInputExplorer::peekInvariantStack() {
  return invariantStack_[invariantStackIdx_ - 1];
}

inline void OutputToInputExplorer::markStable(Timestamp t, VarId v) {
  varStableAt[v] = t;
}

inline bool OutputToInputExplorer::isStable(Timestamp t, VarId v) {
  return varStableAt.at(v) == t;
}

inline bool OutputToInputExplorer::isStable(Timestamp t, InvariantId v) {
  return invariantStableAt.at(v) == t;
}
