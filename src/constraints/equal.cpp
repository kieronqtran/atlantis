#include "constraints/equal.hpp"

#include "core/engine.hpp"

/**
 * Constraint a*x = b*y
 * @param violationId id for the violationCount
 * @param a coefficient of x
 * @param x variable of lhs
 * @param b coefficient of y
 * @param y variable of rhs
 */
Equal::Equal(VarId violationId, VarId x, VarId y)
    : Constraint(NULL_ID, violationId), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void Equal::init(Timestamp, Engine& engine) {
  assert(_id != NULL_ID);

  engine.registerInvariantParameter(_id, _x, LocalId(0));
  engine.registerInvariantParameter(_id, _y, LocalId(0));
  registerDefinedVariable(engine, _violationId);
}

void Equal::recompute(Timestamp ts, Engine& engine) {
  updateValue(ts, engine, _violationId,
              std::abs(engine.getValue(ts, _x) - engine.getValue(ts, _y)));
}

void Equal::notifyIntChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId Equal::getNextParameter(Timestamp ts, Engine&) {
  _state.incValue(ts, 1);
  // todo: maybe this can be faster by first checking null and then doing
  // ==0?m_x:m_y;
  switch (_state.getValue(ts)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void Equal::notifyCurrentParameterChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void Equal::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}
