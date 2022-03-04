#include "invariants/absDiff.hpp"

#include "core/engine.hpp"

AbsDiff::AbsDiff(VarId x, VarId y, VarId absDiff)
    : Invariant(NULL_ID), _x(x), _y(y), _absDiff(absDiff) {
  _modifiedVars.reserve(1);
}

void AbsDiff::init([[maybe_unused]] Timestamp ts, Engine& engine) {
  assert(!_id.equals(NULL_ID));

  registerDefinedVariable(engine, _absDiff);
  engine.registerInvariantInput(_id, _x, 0);
  engine.registerInvariantInput(_id, _y, 0);
}

void AbsDiff::recompute(Timestamp ts, Engine& engine) {
  updateValue(ts, engine, _absDiff,
              std::abs(engine.value(ts, _x) - engine.value(ts, _y)));
}

void AbsDiff::notifyIntChanged(Timestamp ts, Engine& engine, LocalId) {
  recompute(ts, engine);
}

VarId AbsDiff::nextInput(Timestamp ts, Engine&) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void AbsDiff::notifyCurrentInputChanged(Timestamp ts, Engine& engine) {
  recompute(ts, engine);
}

void AbsDiff::commit(Timestamp ts, Engine& engine) {
  Invariant::commit(ts, engine);
}
