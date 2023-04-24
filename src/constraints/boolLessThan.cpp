#include "constraints/boolLessThan.hpp"

/**
 * Constraint x = y
 * @param violationId id for the violationCount
 * @param x variable of lhs
 * @param y variable of rhs
 */
BoolLessThan::BoolLessThan(Engine& engine, VarId violationId, VarId x, VarId y)
    : Constraint(engine, violationId), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void BoolLessThan::registerVars() {
  assert(_id != NULL_ID);
  _engine.registerInvariantInput(_id, _x, LocalId(0));
  _engine.registerInvariantInput(_id, _y, LocalId(0));
  registerDefinedVariable(_violationId);
}

void BoolLessThan::updateBounds(bool widenOnly) {
  _engine.updateBounds(_violationId, 0, 1, widenOnly);
}

void BoolLessThan::recompute(Timestamp ts) {
  updateValue(
      ts, _violationId,
      static_cast<Int>(_engine.value(ts, _x) == 0) + _engine.value(ts, _y));
}

void BoolLessThan::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }

VarId BoolLessThan::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void BoolLessThan::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }
