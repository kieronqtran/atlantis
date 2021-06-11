
#include "propagation/outputToInputExplorer.hpp"

#include "core/propagationEngine.hpp"

OutputToInputExplorer::OutputToInputExplorer(PropagationEngine& e,
                                             size_t expectedSize)
    : m_engine(e),
      varStackIdx_(0),
      invariantStackIdx_(0),
      varStableAt(expectedSize),
      invariantStableAt(expectedSize),
      invariantIsOnStack(expectedSize),
      m_decisionVarAncestor(expectedSize) {
  variableStack_.reserve(expectedSize);
  invariantStack_.reserve(expectedSize);
}

void OutputToInputExplorer::populateAncestors() {
  std::vector<bool> varVisited(m_engine.getNumVariables() + 1);
  std::deque<IdBase> stack(m_engine.getNumVariables() + 1);

  for (IdBase idx = 1; idx <= m_engine.getNumVariables(); ++idx) {
    if (m_decisionVarAncestor.size() < idx) {
      m_decisionVarAncestor.register_idx(idx);
    }

    m_decisionVarAncestor[idx].clear();
  }

  for (const VarIdBase decisionVar : m_engine.getDecisionVariables()) {
    std::fill(varVisited.begin(), varVisited.end(), false);
    stack.clear();
    stack.push_back(decisionVar);
    varVisited[decisionVar] = true;

    while (stack.size() > 0) {
      const VarIdBase id = stack.back();
      stack.pop_back();
      m_decisionVarAncestor[id].emplace(decisionVar);

      for (InvariantId invariantId :
           m_engine.getListeningInvariants(IdBase(id))) {
        for (VarIdBase outputVar :
             m_engine.getVariablesDefinedBy(invariantId)) {
          if (!varVisited[outputVar]) {
            varVisited[outputVar] = true;
            stack.push_back(outputVar);
          }
        }
      }
    }
  }
}

template bool OutputToInputExplorer::isUpToDate<true>(VarIdBase id);
template bool OutputToInputExplorer::isUpToDate<false>(VarIdBase id);
template <bool OutputToInputMarking>
bool OutputToInputExplorer::isUpToDate(VarIdBase id) {
  if constexpr (OutputToInputMarking) {
    for (const size_t ancestor : m_engine.getModifiedDecisionVariables()) {
      if (m_decisionVarAncestor.at(id).find(ancestor) !=
          m_decisionVarAncestor.at(id).end()) {
        return false;
      }
    }
    return true;
  } else {
    return !m_engine.isOnPropagationPath(id);
  }
}

template void OutputToInputExplorer::preprocessVarStack<false>(
    Timestamp currentTime);
template void OutputToInputExplorer::preprocessVarStack<true>(
    Timestamp currentTime);
template <bool OutputToInputMarking>
void OutputToInputExplorer::preprocessVarStack(Timestamp currentTime) {
  size_t newStackSize = 0;
  for (size_t s = 0; s < varStackIdx_; ++s) {
    if (!isUpToDate<OutputToInputMarking>(variableStack_[s])) {
      variableStack_[newStackSize] = variableStack_[s];
      ++newStackSize;
    } else {
      markStable(currentTime, variableStack_[s]);
    }
  }
  varStackIdx_ = newStackSize;
}

template void OutputToInputExplorer::expandInvariant<true>(InvariantId inv);
template void OutputToInputExplorer::expandInvariant<false>(InvariantId inv);

// We expand an invariant by pushing it and its first input variable onto each
// stack.
template <bool OutputToInputMarking>
void OutputToInputExplorer::expandInvariant(InvariantId inv) {
  if (inv == NULL_ID) {
    return;
  }
  if (invariantIsOnStack.get(inv)) {
    throw DynamicCycleException();
  }
  VarId nextVar = m_engine.getNextDependency(inv);
  while (nextVar != NULL_ID && isUpToDate<OutputToInputMarking>(nextVar)) {
    nextVar = m_engine.getNextDependency(inv);
  }

  if (nextVar.id == NULL_ID) {
    return;
  }
  pushVariableStack(nextVar);
  pushInvariantStack(inv);
}

void OutputToInputExplorer::notifyCurrentInvariant() {
  m_engine.notifyCurrentDependencyChanged(peekInvariantStack());
}

template bool OutputToInputExplorer::pushNextInputVariable<true>();
template bool OutputToInputExplorer::pushNextInputVariable<false>();

template <bool OutputToInputMarking>
bool OutputToInputExplorer::pushNextInputVariable() {
  VarId nextVar = m_engine.getNextDependency(peekInvariantStack());
  while (nextVar != NULL_ID && isUpToDate<OutputToInputMarking>(nextVar)) {
    nextVar = m_engine.getNextDependency(peekInvariantStack());
  }
  if (nextVar.id == NULL_ID) {
    return true;  // done with invariant
  }
  pushVariableStack(nextVar);
  return false;  // not done with invariant
}

void OutputToInputExplorer::registerVar(VarId id) {
  variableStack_.emplace_back(NULL_ID);  // push back just to resize the stack!
  varStableAt.register_idx(id);
}

void OutputToInputExplorer::registerInvariant(InvariantId id) {
  invariantStack_.emplace_back(NULL_ID);  // push back just to resize the stack!
  invariantStableAt.register_idx(id);
  invariantIsOnStack.register_idx(id, false);
}

template void OutputToInputExplorer::propagate<true>(Timestamp currentTime);
template void OutputToInputExplorer::propagate<false>(Timestamp currentTime);

template <bool OutputToInputMarking>
void OutputToInputExplorer::propagate(Timestamp currentTime) {
  preprocessVarStack<OutputToInputMarking>(currentTime);
  // recursively expand variables to compute their value.
  while (varStackIdx_ > 0) {
    VarId currentVar = peekVariableStack();

    // If the variable is not stable, then expand it.
    if (!isStable(currentTime, currentVar)) {
      // Variable will become stable as it is either not defined or we now
      // expand its invariant. Note that expandInvariant may sometimes not
      // push a new invariant nor a new variable on the stack, so we must mark
      // the variable as stable before we expand it as this otherwise results in
      // an infinite loop.
      markStable(currentTime, currentVar);
      // The variable is upToDate and stable: expand its defining invariant.
      expandInvariant<OutputToInputMarking>(
          m_engine.getDefiningInvariant(currentVar));
      continue;
    }
    // currentVar is done: pop it from the stack.
    popVariableStack();
    if (invariantStackIdx_ == 0) {
      // we are at an output variable that is already
      // stable. Just continue!
      continue;
    }
    if (m_engine.hasChanged(currentTime, currentVar)) {
      // If the variable is stable and has changed then just send a
      // notification to top invariant (i.e, the one asking for its value)
      notifyCurrentInvariant();
    }
    // push the next input variable of the top invariant
    bool invariantDone = pushNextInputVariable<OutputToInputMarking>();
    if (invariantDone) {
      // The top invariant has finished propagating, so all defined vars can
      // be marked as stable at the current time.
      for (auto defVar : m_engine.getVariablesDefinedBy(peekInvariantStack())) {
        markStable(currentTime, defVar);
      }
      popInvariantStack();
    }
  }
  clearRegisteredVariables();
}
