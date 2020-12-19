#include "propagation/propagationGraph.hpp"

PropagationGraph::PropagationGraph(size_t expectedSize)
    : m_numInvariants(0),
      m_numVariables(0),
      m_definingInvariant(expectedSize),
      m_variablesDefinedByInvariant(expectedSize),
      m_inputVariables(expectedSize),
      m_listeningInvariants(expectedSize),
      m_topology(*this) {}

void PropagationGraph::registerInvariant([[maybe_unused]] InvariantId id) {
  // Everything must be registered in sequence.
  m_variablesDefinedByInvariant.register_idx(id);
  m_inputVariables.register_idx(id);
  ++m_numInvariants;
}

void PropagationGraph::registerVar([[maybe_unused]] VarIdBase id) {
  m_listeningInvariants.register_idx(id);
  m_definingInvariant.register_idx(id);
  ++m_numVariables;
}

void PropagationGraph::registerInvariantDependsOnVar(InvariantId dependent,
                                                     VarIdBase source) {
  assert(!dependent.equals(NULL_ID) && !source.equals(NULL_ID));
  if (m_definingInvariant[source] == dependent) {
    // If the dependent invariant defines the source variable (the source
    // variable depends on the dependent invariant), then do nothing.
    // This check was previously done during propagation, but has been
    // moved here for speed reasons.
    // This behaviour should be logged as a warning when it occurs.
    return;
  }
  m_listeningInvariants[source].push_back(dependent);
  m_inputVariables[dependent].push_back(source);
}

void PropagationGraph::registerDefinedVariable(VarIdBase dependent,
                                               InvariantId source) {
  assert(!dependent.equals(NULL_ID) && !source.equals(NULL_ID));
  if (m_definingInvariant.at(dependent).id != NULL_ID.id) {
    throw VariableAlreadyDefinedException(
        "Variable " + std::to_string(dependent.id) +
        " already defined by invariant " +
        std::to_string(m_definingInvariant.at(dependent).id));
  }
  Int index = -1;
  for (size_t i = 0; i < m_listeningInvariants[dependent].size(); ++i) {
    if (m_listeningInvariants[dependent][i] == source) {
      index = i;
      break;
    }
  }
  if (index >= 0) {
    // If the source invariant depends on the  dependent variable, then
    // remove that dependency.
    // This check was previously done during propagation, but has been
    // moved here for speed reasons.
    // This behaviour should be logged as a warning when it occurs.
    m_listeningInvariants[dependent].erase(
      m_listeningInvariants[dependent].begin() + index
    );
  }
  m_definingInvariant[dependent] = source;
  m_variablesDefinedByInvariant[source].push_back(dependent);
}

void PropagationGraph::close() {
  m_isInputVar.resize(getNumVariables() + 1);
  m_isOutputVar.resize(getNumVariables() + 1);
  for (size_t i = 1; i < getNumVariables() + 1; i++) {
    m_isOutputVar.at(i) = (m_listeningInvariants.at(i).empty());
    m_isInputVar.at(i) = (m_definingInvariant.at(i) == NULL_ID);
  }

  m_topology.computeWithCycles();
  //  m_topology.computeNoCycles();
}
