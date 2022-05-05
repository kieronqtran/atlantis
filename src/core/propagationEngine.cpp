#include "core/propagationEngine.hpp"

#include <iostream>

PropagationEngine::PropagationEngine()
    : _propagationMode(PropagationMode::INPUT_TO_OUTPUT),
      _numVariables(0),
      _propGraph(ESTIMATED_NUM_OBJECTS),
      _outputToInputExplorer(*this, ESTIMATED_NUM_OBJECTS),
      _isEnqueued(ESTIMATED_NUM_OBJECTS),
      _modifiedSearchVariables() {}

PropagationGraph& PropagationEngine::propGraph() { return _propGraph; }

void PropagationEngine::open() {
  if (_isOpen) {
    throw EngineOpenException("Engine already open.");
  }
  if (_engineState != EngineState::IDLE) {
    throw EngineStateException("Engine must be idle before opening.");
  }
  _isOpen = true;
}

void PropagationEngine::close() {
  if (!_isOpen) {
    throw EngineClosedException("Engine already closed.");
  }

  incCurrentTimestamp();

  _isOpen = false;
  try {
    _propGraph.close();
  } catch (std::exception const& e) {
    std::cout << "foo";
  }
  if (_propagationMode == PropagationMode::OUTPUT_TO_INPUT) {
    if (outputToInputMarkingMode() == OutputToInputMarkingMode::NONE) {
      _outputToInputExplorer.close<OutputToInputMarkingMode::NONE>();
    } else if (outputToInputMarkingMode() ==
               OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC) {
      _outputToInputExplorer
          .close<OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC>();
    } else if (outputToInputMarkingMode() ==
               OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION) {
      _outputToInputExplorer
          .close<OutputToInputMarkingMode::INPUT_TO_OUTPUT_EXPLORATION>();
    }
  }

#ifndef NDEBUG
  for (const VarIdBase varId : searchVariables()) {
    // Assert that if search variable varId is modified,
    // then it is in the set of modified search variables
    assert(_store.intVar(varId).hasChanged(_currentTimestamp) ==
           _modifiedSearchVariables.contains(varId));
  }
#endif

  // close all invariants
  closeInvariants();

  // compute initial values for variables and for (internal datastructure of)
  // invariants
  recomputeAndCommit();

#ifndef NDEBUG
  for (const size_t varId : _modifiedSearchVariables) {
    // assert that decsion variable varId is no longer modified.
    assert(!_store.intVar(varId).hasChanged(_currentTimestamp));
  }
#endif
}

//---------------------Registration---------------------
void PropagationEngine::enqueueComputedVar(Timestamp, VarId id) {
  if (_isEnqueued.get(id)) {
    return;
  }
  _propGraph.enqueuePropagationQueue(id);
  _isEnqueued.set(id, true);
}

void PropagationEngine::registerInvariantInput(InvariantId invariantId,
                                               VarId inputId, LocalId localId) {
  assert(localId < _store.invariant(invariantId).notifiableVarsSize());
  const auto id = sourceId(inputId);
  _propGraph.registerInvariantInput(invariantId, id);
  _listeningInvariantData[id].emplace_back(
      ListeningInvariantData{invariantId, localId});
}

void PropagationEngine::registerDefinedVariable(VarId varId,
                                                InvariantId invariantId) {
  _propGraph.registerDefinedVariable(sourceId(varId), invariantId);
}

void PropagationEngine::registerVar(VarId id) {
  _numVariables++;
  _propGraph.registerVar(id);
  _outputToInputExplorer.registerVar(id);
  _isEnqueued.register_idx(id, false);
}

void PropagationEngine::registerInvariant(InvariantId invariantId) {
  _propGraph.registerInvariant(invariantId);
  _outputToInputExplorer.registerInvariant(invariantId);
}

//---------------------Propagation---------------------

VarId PropagationEngine::dequeueComputedVar(Timestamp) {
  assert(propagationMode() == PropagationMode::INPUT_TO_OUTPUT ||
         _engineState == EngineState::COMMIT);
  if (_propGraph.propagationQueueEmpty()) {
    return VarId(NULL_ID);
  }
  VarId nextVar(_propGraph.dequeuePropagationQueue());
  _isEnqueued.set(nextVar, false);
  // Due to enqueueComputedVar, all variables in the queue are "active".
  return nextVar;
}

void PropagationEngine::clearPropagationQueue() {
  _propGraph.clearPropagationQueue();
  _isEnqueued.assign_all(false);
}

void PropagationEngine::closeInvariants() {
  for (auto iter = _store.invariantBegin(); iter != _store.invariantEnd();
       ++iter) {
    (*iter)->close(_currentTimestamp, *this);
  }
}

void PropagationEngine::recomputeAndCommit() {
  // TODO: This is a very inefficient way of initialising!
  size_t tries = 0;
  bool done = false;
  while (!done) {
    done = true;
    if (tries++ > _store.numVariables()) {
      throw FailedToInitialise();
    }
    for (auto iter = _store.invariantBegin(); iter != _store.invariantEnd();
         ++iter) {
      assert((*iter) != nullptr);
      (*iter)->recompute(_currentTimestamp, *this);
    }
    for (auto iter = _store.intVarBegin(); iter != _store.intVarEnd(); ++iter) {
      if (iter->hasChanged(_currentTimestamp)) {
        done = false;
        iter->commit();
      }
    }
  }
  // We must commit all invariants once everything is stable.
  // Commiting an invariant will commit any internal datastructure.
  for (auto iter = _store.invariantBegin(); iter != _store.invariantEnd();
       ++iter) {
    (*iter)->commit(_currentTimestamp, *this);
  }
  clearPropagationQueue();
}

//--------------------- Propagation ---------------------
void PropagationEngine::beginMove() {
  assert(!_isOpen);
  assert(_engineState == EngineState::IDLE);

  incCurrentTimestamp();
  _engineState = EngineState::MOVE;
}

void PropagationEngine::endMove() {
  assert(_engineState == EngineState::MOVE);
  _engineState = EngineState::IDLE;
}

void PropagationEngine::beginProbe() {
  assert(!_isOpen);
  assert(_engineState == EngineState::IDLE);
  _engineState = EngineState::PROBE;
}

void PropagationEngine::query(VarId id) {
  assert(!_isOpen);
  assert(_engineState != EngineState::IDLE &&
         _engineState != EngineState::PROCESSING);

  if (_propagationMode != PropagationMode::INPUT_TO_OUTPUT) {
    _outputToInputExplorer.registerForPropagation(_currentTimestamp,
                                                  sourceId(id));
  }
}

void PropagationEngine::endProbe() {
  assert(_engineState == EngineState::PROBE);

  _engineState = EngineState::PROCESSING;
  try {
    if (_propagationMode == PropagationMode::INPUT_TO_OUTPUT) {
      propagate<false>();
    } else {
#ifndef NDEBUG
      if (outputToInputMarkingMode() ==
          OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC) {
        for (VarIdBase varId : searchVariables()) {
          // Assert that if decision variable varId is modified,
          // then it is in the set of modified decision variables
          assert(_store.intVar(varId).hasChanged(_currentTimestamp) ==
                 _modifiedSearchVariables.contains(varId));
        }
      }
#endif
      outputToInputPropagate();
    }
    _engineState = EngineState::IDLE;
  } catch (std::exception const& e) {
    _engineState = EngineState::IDLE;
    throw e;
  }
}

void PropagationEngine::beginCommit() {
  assert(!_isOpen);
  assert(_engineState == EngineState::IDLE);

  _outputToInputExplorer.clearRegisteredVariables();

  _engineState = EngineState::COMMIT;
}

void PropagationEngine::endCommit() {
  assert(_engineState == EngineState::COMMIT);

  _engineState = EngineState::PROCESSING;

  try {
#ifndef NDEBUG
    if (_propagationMode == PropagationMode::OUTPUT_TO_INPUT &&
        outputToInputMarkingMode() ==
            OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC) {
      for (VarIdBase varId : searchVariables()) {
        // Assert that if decision variable varId is modified,
        // then it is in the set of modified decision variables
        assert(_store.intVar(varId).hasChanged(_currentTimestamp) ==
               _modifiedSearchVariables.contains(varId));
      }
    }
#endif

    propagate<true>();

#ifndef NDEBUG
    if (_propagationMode == PropagationMode::OUTPUT_TO_INPUT &&
        outputToInputMarkingMode() ==
            OutputToInputMarkingMode::OUTPUT_TO_INPUT_STATIC) {
      for (size_t varId : _modifiedSearchVariables) {
        // assert that decsion variable varId is no longer modified.
        assert(!_store.intVar(varId).hasChanged(_currentTimestamp));
      }
    }
#endif
    _engineState = EngineState::IDLE;
  } catch (std::exception const& e) {
    _engineState = EngineState::IDLE;
    throw e;
  }
}

template void PropagationEngine::propagate<true>();
template void PropagationEngine::propagate<false>();

// Propagates at the current internal timestamp of the engine.
template <bool DoCommit>
void PropagationEngine::propagate() {
  for (VarId queuedVar = dequeueComputedVar(_currentTimestamp);
       queuedVar.id != NULL_ID;
       queuedVar = dequeueComputedVar(_currentTimestamp)) {
    // queuedVar has been computed under _currentTimestamp
    const IntVar& variable = _store.intVar(queuedVar);

    const InvariantId definingInvariant =
        _propGraph.definingInvariant(queuedVar);

    if (definingInvariant != NULL_ID) {
      Invariant& defInv = _store.invariant(definingInvariant);
      if (queuedVar == defInv.primaryDefinedVar()) {
        const Int oldValue = variable.value(_currentTimestamp);
        defInv.compute(_currentTimestamp, *this);
        for (const VarId inputId : defInv.nonPrimaryDefinedVars()) {
          if (hasChanged(_currentTimestamp, inputId)) {
            enqueueComputedVar(_currentTimestamp, inputId);
          }
        }
        if constexpr (DoCommit) {
          defInv.commit(_currentTimestamp, *this);
        }
        if (oldValue == variable.value(_currentTimestamp)) {
          continue;
        }
      }
    }

    if constexpr (DoCommit) {
      commitIf(_currentTimestamp, queuedVar);
    }

    for (const auto& toNotify : _listeningInvariantData[queuedVar]) {
      Invariant& invariant = _store.invariant(toNotify.invariantId);
      invariant.notify(toNotify.localId);
      enqueueComputedVar(_currentTimestamp, invariant.primaryDefinedVar());
    }
  }
}

void PropagationEngine::computeBounds() {
  IdMap<InvariantId, Int> inputsToCompute(numInvariants());

  for (size_t invariantId = 1u; invariantId <= numInvariants(); ++invariantId) {
    inputsToCompute.register_idx(invariantId,
                                 inputVariables(invariantId).size());
  }

  // Search variables might now have been computed yet
  for (size_t varId = 1u; varId <= numVariables(); ++varId) {
    if (definingInvariant(varId) == NULL_ID) {
      for (const InvariantId listeningInvariantId :
           listeningInvariants(varId)) {
        --inputsToCompute[listeningInvariantId];
      }
    }
  }

  auto cmp = [&](InvariantId a, InvariantId b) {
    if (inputsToCompute[a] == inputsToCompute[b]) {
      return a < b;
    }
    return inputsToCompute[a] < inputsToCompute[b];
  };

  std::set<InvariantId, decltype(cmp)> invariantQueue(cmp);

  for (size_t invariantId = 1; invariantId <= numInvariants(); ++invariantId) {
    invariantQueue.emplace(invariantId);
  }

  while (!invariantQueue.empty()) {
    const InvariantId invariantId = *invariantQueue.begin();
    assert(inputsToCompute[invariantId] >= 0);

    invariantQueue.erase(invariantId);
    assert(!invariantQueue.contains(invariantId));

    assert(!invariantQueue.contains(invariantId));

#ifndef NDEBUG
    for (const InvariantId invId : invariantQueue) {
      // If the following assertion fails, then inputsToCompute[i] was updated
      // before removing invariant i:
      assert(invId != invariantId);
    }
    for (const InvariantId invId : invariantQueue) {
      assert(inputsToCompute[invariantId] < inputsToCompute[invId] ||
             (inputsToCompute[invariantId] == inputsToCompute[invId] &&
              size_t(invariantId) <= size_t(invId)));
    }
#endif
    _store.invariant(invariantId).updateBounds(*this, true);

    for (const VarIdBase outputVarId :
         _propGraph.variablesDefinedBy(invariantId)) {
      for (const InvariantId listeningInvariantId :
           listeningInvariants(outputVarId)) {
        // Remove from the data structure must happen before updating
        // inputsToCompute
        if (invariantQueue.contains(listeningInvariantId)) {
          invariantQueue.erase(listeningInvariantId);
        }
        assert(listeningInvariantId != invariantId);
        --inputsToCompute[listeningInvariantId];

        if (inputsToCompute[listeningInvariantId] >= 0) {
          invariantQueue.emplace(listeningInvariantId);
        }
      }
    }
  }
}