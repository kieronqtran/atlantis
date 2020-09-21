#pragma once

#include "core/savedInt.hpp"
#include "core/types.hpp"
class Engine;  // Forward declaration

class Invariant {
 private:
 protected:
  bool m_isPostponed;
  InvariantId m_id;
  // State used for returning next dependency. Null state is -1 by default
  SavedInt m_state;
  Invariant(Id t_id) : m_isPostponed(false), m_id(t_id), m_state(NULL_TIMESTAMP, -1) {}
  Invariant(Id t_id, Int nullState)
      : m_id(t_id), m_state(NULL_TIMESTAMP, nullState) {}

 public:
  virtual ~Invariant() {}

  void setId(Id t_id) { m_id = t_id; }

  /**
   * Preconditions for initialisation:
   * 1) The invariant has been registered in an engine and has a valid ID.
   *
   * 2) All variables have valid ids (i.e., they have been
   * registered)
   *
   * Checklist for initialising an invariant:
   *
   *
   * 2) Register any output variables that are defined by this
   * invariant note that this can throw an exception if such a variable is
   * already defined.
   *
   * 3) Register dependency to any input variables.
   *
   * 4) Compute initial state of invariant!
   */
  virtual void init(Timestamp, Engine&) = 0;

  virtual void recompute(Timestamp, Engine&) = 0;

  virtual VarId getNextDependency(Timestamp, Engine& e) = 0;

  virtual void notifyCurrentDependencyChanged(Timestamp, Engine& e) = 0;

  /**
   * Precondition: oldValue != newValue
   */
  virtual void notifyIntChanged(Timestamp t, Engine& e, LocalId id,
                                Int newValue) = 0;

  virtual void commit(Timestamp, Engine&){
    m_isPostponed = false;
  };
  inline void postpone(){
    m_isPostponed = true;
  }
  inline bool isPostponed(){
    return m_isPostponed;
  }
};