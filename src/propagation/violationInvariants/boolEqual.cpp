#include "atlantis/propagation/violationInvariants/boolEqual.hpp"

namespace atlantis::propagation {

/**
 * Constraint x = y
 * @param violationId id for the violationCount
 * @param x variable of lhs
 * @param y variable of rhs
 */
BoolEqual::BoolEqual(SolverBase& solver, VarId violationId, VarViewId x,
                     VarViewId y)
    : ViolationInvariant(solver, violationId), _x(x), _y(y) {}

BoolEqual::BoolEqual(SolverBase& solver, VarViewId violationId, VarViewId x,
                     VarViewId y)
    : BoolEqual(solver, VarId(violationId), x, y) {
  assert(violationId.isVar());
}

void BoolEqual::registerVars() {
  assert(_id != NULL_ID);
  _solver.registerInvariantInput(_id, _x, LocalId(0), false);
  _solver.registerInvariantInput(_id, _y, LocalId(0), false);
  registerDefinedVar(_violationId);
}

void BoolEqual::updateBounds(bool widenOnly) {
  _solver.updateBounds(_violationId, 0, 1, widenOnly);
}

void BoolEqual::recompute(Timestamp ts) {
  updateValue(ts, _violationId,
              static_cast<Int>((_solver.value(ts, _x) != 0) !=
                               (_solver.value(ts, _y) != 0)));
}

void BoolEqual::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }

VarViewId BoolEqual::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void BoolEqual::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }
}  // namespace atlantis::propagation
