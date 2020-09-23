#include "core/propagationEngine.hpp"

PropagationEngine::PropagationEngine()
    : m_propGraph(ESTIMATED_NUM_OBJECTS),
      m_bottomUpExplorer(*this, ESTIMATED_NUM_OBJECTS),
      m_modifiedVariables(PropagationGraph::PriorityCmp(m_propGraph)),
      m_isEnqueued() {
  m_isEnqueued.push_back(false);  // initialise NULL_ID for indexing
}

PropagationGraph& PropagationEngine::getPropGraph() { return m_propGraph; }

void PropagationEngine::open() { m_isOpen = true; }

void PropagationEngine::close() {
  ++m_currentTime;  // todo: Is it safe to increment time here? What if a user
                    // tried to change a variable but without a begin move?
                    // But we should ignore it anyway then...
  m_isOpen = false;
  m_propGraph.close();
  // compute initial values for variables and for (internal datastructure of)
  // invariants
  recomputeAndCommit();
}

//---------------------Registration---------------------
void PropagationEngine::notifyMaybeChanged(Timestamp, VarId id) {
  if (m_isEnqueued.at(id)) {
    return;
  }
  m_modifiedVariables.push(id);
  m_isEnqueued.at(id) = true;
}

void PropagationEngine::registerInvariantDependsOnVar(InvariantId dependent,
                                                      VarId source,
                                                      LocalId localId) {
  m_propGraph.registerInvariantDependsOnVar(dependent, source);
  m_dependentInvariantData.at(source).emplace_back(
      InvariantDependencyData{dependent, localId, NULL_TIMESTAMP});
}

void PropagationEngine::registerDefinedVariable(VarId dependent,
                                                InvariantId source) {
  m_propGraph.registerDefinedVariable(dependent, source);
}

void PropagationEngine::registerVar(VarId v) {
  m_propGraph.registerVar(v);
  m_bottomUpExplorer.registerVar(v);
  m_isEnqueued.push_back(false);
}

void PropagationEngine::registerInvariant(InvariantId i) {
  m_propGraph.registerInvariant(i);
  m_bottomUpExplorer.registerInvariant(i);
}

//---------------------Propagation---------------------

VarId PropagationEngine::getNextStableVariable(Timestamp) {
  if (m_modifiedVariables.empty()) {
    return VarId(NULL_ID);
  }
  VarId nextVar = m_modifiedVariables.top();
  m_modifiedVariables.pop();
  m_isEnqueued.at(nextVar) = false;
  // Due to notifyMaybeChanged, all variables in the queue are "active".
  return nextVar;
}

void PropagationEngine::recomputeAndCommit() {
  // TODO: This is a very inefficient way of initialising!
  size_t tries = 0;
  bool done = false;
  while (!done) {
    done = true;
    if (tries++ > m_store.getNumVariables()) {
      throw FailedToInitialise();
    }
    for (auto iter = m_store.invariantBegin(); iter != m_store.invariantEnd();
         ++iter) {
      assert((*iter) != nullptr);
      (*iter)->recompute(m_currentTime, *this);
    }
    for (auto iter = m_store.intVarBegin(); iter != m_store.intVarEnd();
         ++iter) {
      if (iter->hasChanged(m_currentTime)) {
        done = false;
        iter->commit();
      }
    }
  }
  // We must commit all invariants once everything is stable.
  // Commiting an invariant will commit any internal datastructure.
  for (auto iter = m_store.invariantBegin(); iter != m_store.invariantEnd();
       ++iter) {
    (*iter)->commit(m_currentTime, *this);
  }
}

//--------------------- Move semantics ---------------------
void PropagationEngine::beginMove() { ++m_currentTime; }

void PropagationEngine::endMove() {}

void PropagationEngine::beginQuery() {}

void PropagationEngine::query(VarId id) {
  m_bottomUpExplorer.registerForPropagation(m_currentTime, id);
}

void PropagationEngine::endQuery() {
  // m_bottomUpExplorer.schedulePropagation(m_currentTime, *this);
  // propagate();
  // m_bottomUpExplorer.propagate(m_currentTime);
  bottomUpPropagate();
}

void PropagationEngine::beginCommit() {}

void PropagationEngine::endCommit() {
  // Todo: perform top down propagation
  bottomUpPropagate();
  // propagate();
}

// Propagates at the current internal time of the engine.
void PropagationEngine::propagate() {
  VarId id = getNextStableVariable(m_currentTime);
  while (id.id != NULL_ID) {
    IntVar& variable = m_store.getIntVar(id);
    if (variable.hasChanged(m_currentTime)) {
      for (auto& toNotify : m_dependentInvariantData.at(id)) {
        // If we do multiple "probes" within the same timestamp then the
        // invariant may already have been notified.
        // Also, do not notify invariants that are not active.
        if (!isActive(m_currentTime, toNotify.id)) {
          continue;
        }
        m_store.getInvariant(toNotify.id)
            .notifyIntChanged(m_currentTime, *this, toNotify.localId,
                              variable.getValue(m_currentTime));
        toNotify.lastNotification = m_currentTime;
      }
    }
    id = getNextStableVariable(m_currentTime);
  }
}

void PropagationEngine::bottomUpPropagate() {
  m_bottomUpExplorer.propagate(m_currentTime);
}
