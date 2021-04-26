#include "invariants/elementVar.hpp"

ElementVar::ElementVar(VarId i, std::vector<VarId> X, VarId b)
    : Invariant(NULL_ID), m_i(i), m_X(std::move(X)), m_b(b) {
  m_modifiedVars.reserve(1);
}

void ElementVar::init([[maybe_unused]] Timestamp t, Engine& e) {
  assert(m_id != NULL_ID);

  registerDefinedVariable(e, m_b);
  e.registerInvariantDependsOnVar(m_id, m_i, LocalId(0));
  for (size_t i = 0; i < m_X.size(); ++i) {
    e.registerInvariantDependsOnVar(m_id, m_X[i], LocalId(0));
  }
}

void ElementVar::recompute(Timestamp t, Engine& e) {
  updateValue(
      t, e, m_b,
      e.getValue(t, m_X.at(static_cast<unsigned long>(e.getValue(t, m_i)))));
}

void ElementVar::notifyIntChanged(Timestamp t, Engine& e, LocalId) {
  recompute(t, e);
}

VarId ElementVar::getNextDependency(Timestamp t, Engine& e) {
  m_state.incValue(t, 1);
  if (m_state.getValue(t) == 0) {
    return m_i;
  } else if (m_state.getValue(t) == 1) {
    return m_X.at(static_cast<unsigned long>(e.getValue(t, m_i)));
  } else {
    return NULL_ID;  // Done
  }
}

void ElementVar::notifyCurrentDependencyChanged(Timestamp t, Engine& e) {
  recompute(t, e);
}

void ElementVar::commit(Timestamp t, Engine& e) { Invariant::commit(t, e); }
