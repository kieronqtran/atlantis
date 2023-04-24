#include "invariants/pow.hpp"

Pow::Pow(Engine& engine, VarId output, VarId x, VarId y)
    : Invariant(engine), _output(output), _x(x), _y(y) {
  _modifiedVars.reserve(1);
}

void Pow::registerVars() {
  assert(!_id.equals(NULL_ID));

  _engine.registerInvariantInput(_id, _x, 0);
  _engine.registerInvariantInput(_id, _y, 0);
  registerDefinedVariable(_output);
}

void Pow::updateBounds(bool widenOnly) {
  const Int xLb = _engine.lowerBound(_x);
  const Int xUb = _engine.upperBound(_x);

  const Int yLb = _engine.lowerBound(_y);
  const Int yUb = _engine.upperBound(_y);

  Int lb = std::numeric_limits<Int>::max();
  Int ub = std::numeric_limits<Int>::min();

  for (const Int aBound : std::array<Int, 2>{xLb, xUb}) {
    for (const Int bBound : std::array<Int, 2>{yLb, yUb}) {
      if (aBound != 0 || bBound >= 0) {
        lb = std::min<Int>(lb, std::pow(aBound, bBound));
        ub = std::max<Int>(ub, std::pow(aBound, bBound));
      } else {
        if (xLb < 0) {
          lb = std::min<Int>(lb, std::pow(-1, bBound));
          ub = std::max<Int>(ub, std::pow(-1, bBound));
        }
        if (xUb > 0) {
          lb = std::min<Int>(lb, std::pow(1, bBound));
          ub = std::max<Int>(ub, std::pow(1, bBound));
        }
      }
    }
  }

  _zeroReplacement = 1;
  // If base can be 0:
  if (xLb <= 0 && 0 <= xUb) {
    lb = std::min<Int>(lb, 0);
    ub = std::max<Int>(ub, 0);

    // If base can be 0 and exponent can be negative:
    if (yLb < 0) {
      // x^-1 is in {-1, 0, 1}
      lb = std::min<Int>(lb, 1);
      ub = std::max<Int>(ub, 1);
      // can be -1 if base is negative
      if (xLb < 0) {
        lb = std::min<Int>(lb, -1);
        ub = std::max<Int>(ub, -1);
      }
      if (xUb < 0) {
        _zeroReplacement = -1;
      }
    }
  }

  if (xLb < 0 && yUb >= 2) {
    lb = std::min<Int>(lb, std::pow(xLb, yUb - (yUb % 2 == 0)));
    ub = std::max<Int>(ub, std::pow(xLb, yUb - (yUb % 2 == 1)));
  }

  _engine.updateBounds(_output, lb, ub, widenOnly);
}

void Pow::recompute(Timestamp ts) {
  const Int xVal = _engine.value(ts, _x);
  const Int yVal = _engine.value(ts, _y);
  if (xVal == 0 && yVal < 0) {
    updateValue(ts, _output, std::pow(_zeroReplacement, yVal));
    return;
  }
  updateValue(ts, _output, std::pow(xVal, yVal));
}

VarId Pow::nextInput(Timestamp ts) {
  switch (_state.incValue(ts, 1)) {
    case 0:
      return _x;
    case 1:
      return _y;
    default:
      return NULL_ID;
  }
}

void Pow::notifyCurrentInputChanged(Timestamp ts) { recompute(ts); }

void Pow::notifyInputChanged(Timestamp ts, LocalId) { recompute(ts); }
