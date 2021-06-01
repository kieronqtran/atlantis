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
  enum class Operation { NONE, QUERY, MOVE, COMMIT, PROCESSING };

  static const size_t ESTIMATED_NUM_OBJECTS = 1;

  Timestamp m_currentTime;

  bool m_isOpen = true;

  Operation m_currentOperation = Operation::NONE;

  struct InvariantDependencyData {
    InvariantId id;
    LocalId localId;
  };
  // Map from VarID -> vector of InvariantID
  IdMap<VarId, std::vector<InvariantDependencyData>> m_dependentInvariantData;

  Store m_store;

  void incValue(Timestamp, VarId, Int inc);
  inline void incValue(VarId v, Int val) { incValue(m_currentTime, v, val); }

  void updateValue(Timestamp, VarId, Int val);

  inline void updateValue(VarId v, Int val) {
    updateValue(m_currentTime, v, val);
  }

  inline bool hasChanged(Timestamp, VarId);

  /**
   * Register that 'from' defines variable 'to'. Throws exception if
   * already defined.
   * @param dependent the variable that is defined by the invariant
   * @param source the invariant defining the variable
   * @throw if the variable is already defined by an invariant.
   */
  virtual void registerDefinedVariable(VarId dependent, InvariantId source) = 0;

  friend class Invariant;

 public:
  Engine(/* args */);

  virtual ~Engine() = default;

  virtual void open() = 0;
  virtual void close() = 0;

  inline bool isOpen() const noexcept { return m_isOpen; }
  inline bool isMoving() const noexcept {
    return m_currentOperation == Operation::MOVE;
  }

  //--------------------- Variable ---------------------

  inline VarId getSourceId(VarId id) {
    return id.idType == VarIdType::var ? id : m_store.getIntViewSourceId(id);
    // getSourceId(m_store.getIntView(id).getParentId());
  }

  virtual void queueForPropagation(Timestamp t, VarId id) = 0;
  virtual void notifyMaybeChanged(Timestamp t, VarId id) = 0;

  Int getValue(Timestamp, VarId);
  inline Int getNewValue(VarId v) { return getValue(m_currentTime, v); }

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

  [[nodiscard]] inline Int getLowerBound(VarId v) const {
    if (v.idType == VarIdType::view) {
      return getIntViewLowerBound(v);
    }
    assert(v.idType == VarIdType::var);
    return m_store.getConstIntVar(v).getLowerBound();
  }

  [[nodiscard]] inline Int getIntViewLowerBound(VarId v) const {
    assert(v.idType == VarIdType::view);
    return m_store.getConstIntView(v)->getLowerBound();
  }

  [[nodiscard]] inline Int getUpperBound(VarId v) const {
    if (v.idType == VarIdType::view) {
      return getIntViewUpperBound(v);
    }
    assert(v.idType == VarIdType::var);
    return m_store.getConstIntVar(v).getUpperBound();
  }

  [[nodiscard]] inline Int getIntViewUpperBound(VarId v) const {
    assert(v.idType == VarIdType::view);
    return m_store.getConstIntView(v)->getUpperBound();
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
   * Register that Invariant to depends on variable from depends on dependency
   * @param dependent the invariant that the variable depends on
   * @param source the depending variable
   * @param localId the id of the depending variable in the invariant
   * @param data additional data
   */
  virtual void registerInvariantDependsOnVar(InvariantId dependent,
                                             VarId source, LocalId localId) = 0;

  virtual void registerVar(VarId) = 0;
  virtual void registerInvariant(InvariantId) = 0;

  const Store& getStore();
  [[nodiscard]] Timestamp getCurrentTime() const;
};

template <class T, typename... Args>
std::enable_if_t<std::is_base_of<Invariant, T>::value, std::shared_ptr<T>>
Engine::makeInvariant(Args&&... args) {
  if (!m_isOpen) {
    throw ModelNotOpenException("Cannot make invariant when store is closed.");
  }
  auto invariantPtr = std::make_shared<T>(std::forward<Args>(args)...);

  auto newId = m_store.createInvariantFromPtr(invariantPtr);
  registerInvariant(newId);
  logDebug("Created new invariant with id: " << newId);
  invariantPtr->init(m_currentTime, *this);
  return invariantPtr;
}

template <class T, typename... Args>
std::enable_if_t<std::is_base_of<IntView, T>::value, std::shared_ptr<T>>
Engine::makeIntView(Args&&... args) {
  if (!m_isOpen) {
    throw ModelNotOpenException("Cannot make intView when store is closed.");
  }
  auto viewPtr = std::make_shared<T>(std::forward<Args>(args)...);

  auto newId = m_store.createIntViewFromPtr(viewPtr);
  // We don't actually register views as they are invisible to propagation.

  viewPtr->init(newId, *this);
  return viewPtr;
}

template <class T, typename... Args>
std::enable_if_t<std::is_base_of<Constraint, T>::value, std::shared_ptr<T>>
Engine::makeConstraint(Args&&... args) {
  if (!m_isOpen) {
    throw ModelNotOpenException("Cannot make invariant when store is closed.");
  }
  auto constraintPtr = std::make_shared<T>(std::forward<Args>(args)...);

  auto newId = m_store.createInvariantFromPtr(constraintPtr);
  registerInvariant(newId);  // A constraint is a type of invariant.
  logDebug("Created new Constraint with id: " << newId);
  constraintPtr->init(m_currentTime, *this);
  return constraintPtr;
}

//--------------------- Inlined functions ---------------------

inline const Store& Engine::getStore() { return m_store; }
inline Timestamp Engine::getCurrentTime() const { return m_currentTime; }

inline bool Engine::hasChanged(Timestamp t, VarId v) {
  return m_store.getIntVar(v).hasChanged(t);
}

inline Int Engine::getValue(Timestamp t, VarId v) {
  if (v.idType == VarIdType::view) {
    return getIntViewValue(t, v);
  }
  return m_store.getIntVar(v).getValue(t);
}

inline Int Engine::getIntViewValue(Timestamp t, VarId v) {
  return m_store.getIntView(v).getValue(t);
}

inline Int Engine::getCommittedValue(VarId v) {
  if (v.idType == VarIdType::view) {
    return getIntViewCommittedValue(v);
  }
  return m_store.getIntVar(v).getCommittedValue();
}

inline Int Engine::getIntViewCommittedValue(VarId v) {
  return m_store.getIntView(v).getCommittedValue();
}

inline Timestamp Engine::getTmpTimestamp(VarId v) {
  if (v.idType == VarIdType::view) {
    return m_store.getIntVar(m_store.getIntViewSourceId(v)).getTmpTimestamp();
  }
  return m_store.getIntVar(v).getTmpTimestamp();
}

inline bool Engine::isPostponed(InvariantId id) {
  return m_store.getInvariant(id).isPostponed();
}

inline void Engine::recompute(InvariantId id) {
  return m_store.getInvariant(id).recompute(m_currentTime, *this);
}

inline void Engine::recompute(Timestamp t, InvariantId id) {
  return m_store.getInvariant(id).recompute(t, *this);
}

inline void Engine::updateValue(Timestamp t, VarId v, Int val) {
  m_store.getIntVar(v).setValue(t, val);
}

inline void Engine::incValue(Timestamp t, VarId v, Int inc) {
  m_store.getIntVar(v).incValue(t, inc);
}

inline void Engine::commit(VarId v) { m_store.getIntVar(v).commit(); }

inline void Engine::commitIf(Timestamp t, VarId v) {
  m_store.getIntVar(v).commitIf(t);
}

inline void Engine::commitValue(VarId v, Int val) {
  m_store.getIntVar(v).commitValue(val);
}

inline void Engine::commitInvariantIf(Timestamp t, InvariantId id) {
  m_store.getInvariant(id).commit(t, *this);
}

inline void Engine::commitInvariant(InvariantId id) {
  m_store.getInvariant(id).commit(m_currentTime, *this);
}