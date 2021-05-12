#pragma once

#include <cassert>
#include <vector>

// #include "constraints/constraint.hpp"
#include "core/types.hpp"
#include "exceptions/exceptions.hpp"
#include "misc/logging.hpp"
#include "store/store.hpp"
#include "utils/idMap.hpp"
// #include "variables/intVar.hpp"
// #include "views/intView.hpp"

class IntVar;
class IntView;
class Invariant;
class Constraint;

class Engine {
 protected:
  static const size_t ESTIMATED_NUM_OBJECTS = 1;

  Timestamp _currentTime;

  bool _isOpen = true;
  bool _isMoving = false;

  struct ListeningInvariantData {
    InvariantId id;
    LocalId localId;
  };
  // Map from VarID -> vector of InvariantID
  IdMap<VarIdBase, std::vector<ListeningInvariantData>> _listeningInvariantData;

  Store _store;

  void incValue(Timestamp, VarId, Int inc);
  inline void incValue(VarId id, Int val) { incValue(_currentTime, id, val); }

  void updateValue(Timestamp, VarId, Int val);

  inline void updateValue(VarId id, Int val) {
    updateValue(_currentTime, id, val);
  }

  inline bool hasChanged(Timestamp, VarId);

  /**
   * Register that 'from' defines variable 'to'. Throws exception if
   * already defined.
   * @param definedVarId the variable that is defined by the invariant
   * @param invariantId the invariant defining the variable
   * @throw if the variable is already defined by an invariant.
   */
  virtual void registerDefinedVariable(VarId definedVarId,
                                       InvariantId invariantId) = 0;

  friend class Invariant;

 public:
  Engine(/* args */);

  virtual ~Engine() = default;

  virtual void open() = 0;
  virtual void close() = 0;

  inline bool isOpen() const noexcept { return _isOpen; }
  inline bool isMoving() const noexcept { return _isMoving; }

  //--------------------- Variable ---------------------

  inline VarId getSourceId(VarId id) {
    return id.idType == VarIdType::var ? id : _store.getIntViewSourceId(id);
    // getSourceId(_store.getIntView(id).getParentId());
  }

  virtual void queueForPropagation(Timestamp, VarId) = 0;
  virtual void notifyMaybeChanged(Timestamp, VarId) = 0;

  Int getValue(Timestamp, VarId);
  inline Int getNewValue(VarId id) { return getValue(_currentTime, id); }

  Int getIntViewValue(Timestamp, VarId);

  Int getCommittedValue(VarId);

  Int getIntViewCommittedValue(VarId);

  Timestamp getTmpTimestamp(VarId);

  bool isPostponed(InvariantId);

  void recompute(InvariantId);
  void recompute(Timestamp, InvariantId);

  void commit(VarId);  // todo: this feels dangerous, maybe commit should
                       // always have a timestamp?
  void commitIf(Timestamp, VarId);
  void commitValue(VarId, Int val);

  [[nodiscard]] inline Int getLowerBound(VarId id) const {
    if (id.idType == VarIdType::view) {
      return getIntViewLowerBound(id);
    }
    assert(id.idType == VarIdType::var);
    return _store.getConstIntVar(id).getLowerBound();
  }

  [[nodiscard]] inline Int getIntViewLowerBound(VarId id) const {
    assert(id.idType == VarIdType::view);
    return _store.getConstIntView(id)->getLowerBound();
  }

  [[nodiscard]] inline Int getUpperBound(VarId id) const {
    if (id.idType == VarIdType::view) {
      return getIntViewUpperBound(id);
    }
    assert(id.idType == VarIdType::var);
    return _store.getConstIntVar(id).getUpperBound();
  }

  [[nodiscard]] inline Int getIntViewUpperBound(VarId id) const {
    assert(id.idType == VarIdType::view);
    return _store.getConstIntView(id)->getUpperBound();
  }

  void commitInvariantIf(Timestamp, InvariantId);

  void commitInvariant(InvariantId);

  //--------------------- Registration ---------------------
  /**
   * Register an invariant in the engine and return its pointer.
   * This also sets the id of the invariant to the new id.
   * @param args the constructor arguments of the invariant
   * @return the created invariant.
   */
  template <class T, typename... Args>
  std::enable_if_t<std::is_base_of<Invariant, T>::value, std::shared_ptr<T>>
  makeInvariant(Args&&... args);

  /**
   * Register an IntView in the engine and return its pointer.
   * This also sets the id of the IntView to the new id.
   * @param args the constructor arguments of the IntView
   * @return the created IntView.
   */
  template <class T, typename... Args>
  std::enable_if_t<std::is_base_of<IntView, T>::value, std::shared_ptr<T>>
  makeIntView(Args&&... args);

  /**
   * Register a constraint in the engine and return its pointer.
   * This also sets the id of the constraint to the new id.
   * @param args the constructor arguments of the constraint
   * @return the created constraint.
   */
  template <class T, typename... Args>
  std::enable_if_t<std::is_base_of<Constraint, T>::value, std::shared_ptr<T>>
  makeConstraint(Args&&... args);

  /**
   * Creates an IntVar and registers it to the engine.
   * @return the created IntVar
   */
  VarId makeIntVar(Int initValue, Int lowerBound, Int upperBound);

  /**
   * Register that a variable is a parameter to an invariant.
   * @param invariantId the invariant
   * @param varId the parameter
   * @param localId the id of the parameter in the invariant
   */
  virtual void registerInvariantParameter(InvariantId invariantId, VarId varId,
                                          LocalId localId) = 0;

  virtual void registerVar(VarId) = 0;
  virtual void registerInvariant(InvariantId) = 0;

  const Store& getStore();
  [[nodiscard]] Timestamp getCurrentTime() const;
};

template <class T, typename... Args>
std::enable_if_t<std::is_base_of<Invariant, T>::value, std::shared_ptr<T>>
Engine::makeInvariant(Args&&... args) {
  if (!_isOpen) {
    throw ModelNotOpenException("Cannot make invariant when store is closed.");
  }
  auto invariantPtr = std::make_shared<T>(std::forward<Args>(args)...);

  auto newId = _store.createInvariantFromPtr(invariantPtr);
  registerInvariant(newId);
  logDebug("Created new invariant with id: " << newId);
  invariantPtr->init(_currentTime, *this);
  return invariantPtr;
}

template <class T, typename... Args>
std::enable_if_t<std::is_base_of<IntView, T>::value, std::shared_ptr<T>>
Engine::makeIntView(Args&&... args) {
  if (!_isOpen) {
    throw ModelNotOpenException("Cannot make intView when store is closed.");
  }
  auto viewPtr = std::make_shared<T>(std::forward<Args>(args)...);

  auto newId = _store.createIntViewFromPtr(viewPtr);
  // We don'ts actually register views as they are invisible to propagation.

  viewPtr->init(newId, *this);
  return viewPtr;
}

template <class T, typename... Args>
std::enable_if_t<std::is_base_of<Constraint, T>::value, std::shared_ptr<T>>
Engine::makeConstraint(Args&&... args) {
  if (!_isOpen) {
    throw ModelNotOpenException("Cannot make invariant when store is closed.");
  }
  auto constraintPtr = std::make_shared<T>(std::forward<Args>(args)...);

  auto newId = _store.createInvariantFromPtr(constraintPtr);
  registerInvariant(newId);  // A constraint is a type of invariant.
  logDebug("Created new Constraint with id: " << newId);
  constraintPtr->init(_currentTime, *this);
  return constraintPtr;
}

//--------------------- Inlined functions ---------------------

inline const Store& Engine::getStore() { return _store; }
inline Timestamp Engine::getCurrentTime() const { return _currentTime; }

inline bool Engine::hasChanged(Timestamp ts, VarId id) {
  return _store.getIntVar(id).hasChanged(ts);
}

inline Int Engine::getValue(Timestamp ts, VarId id) {
  if (id.idType == VarIdType::view) {
    return getIntViewValue(ts, id);
  }
  return _store.getIntVar(id).getValue(ts);
}

inline Int Engine::getIntViewValue(Timestamp ts, VarId id) {
  return _store.getIntView(id).getValue(ts);
}

inline Int Engine::getCommittedValue(VarId id) {
  if (id.idType == VarIdType::view) {
    return getIntViewCommittedValue(id);
  }
  return _store.getIntVar(id).getCommittedValue();
}

inline Int Engine::getIntViewCommittedValue(VarId id) {
  return _store.getIntView(id).getCommittedValue();
}

inline Timestamp Engine::getTmpTimestamp(VarId id) {
  if (id.idType == VarIdType::view) {
    return _store.getIntVar(_store.getIntViewSourceId(id)).getTmpTimestamp();
  }
  return _store.getIntVar(id).getTmpTimestamp();
}

inline bool Engine::isPostponed(InvariantId id) {
  return _store.getInvariant(id).isPostponed();
}

inline void Engine::recompute(InvariantId id) {
  return _store.getInvariant(id).recompute(_currentTime, *this);
}

inline void Engine::recompute(Timestamp ts, InvariantId id) {
  return _store.getInvariant(id).recompute(ts, *this);
}

inline void Engine::updateValue(Timestamp ts, VarId id, Int val) {
  _store.getIntVar(id).setValue(ts, val);
}

inline void Engine::incValue(Timestamp ts, VarId id, Int inc) {
  _store.getIntVar(id).incValue(ts, inc);
}

inline void Engine::commit(VarId id) { _store.getIntVar(id).commit(); }

inline void Engine::commitIf(Timestamp ts, VarId id) {
  _store.getIntVar(id).commitIf(ts);
}

inline void Engine::commitValue(VarId id, Int val) {
  _store.getIntVar(id).commitValue(val);
}

inline void Engine::commitInvariantIf(Timestamp ts, InvariantId id) {
  _store.getInvariant(id).commit(ts, *this);
}

inline void Engine::commitInvariant(InvariantId id) {
  _store.getInvariant(id).commit(_currentTime, *this);
}