#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>

#include <memory>
#include <vector>

#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/views/equalConst.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class EqualViewConst : public ::testing::Test {
 protected:
  std::shared_ptr<Solver> _solver;

  void SetUp() override { _solver = std::make_unique<Solver>(); }
};

RC_GTEST_FIXTURE_PROP(EqualViewConst, simple, (int a, int b)) {
  if (!_solver->isOpen()) {
    _solver->open();
  }
  const VarViewId varId = _solver->makeIntVar(a, a, a);
  const VarViewId violationId =
      _solver->makeIntView<EqualConst>(*_solver, varId, b);
  RC_ASSERT(_solver->committedValue(violationId) == std::abs(Int(a) - Int(b)));
}

RC_GTEST_FIXTURE_PROP(EqualViewConst, singleton, (int a, int b)) {
  if (!_solver->isOpen()) {
    _solver->open();
  }
  const VarViewId varId = _solver->makeIntVar(a, a, a);
  const VarViewId violationId =
      _solver->makeIntView<EqualConst>(*_solver, varId, b);
  RC_ASSERT(_solver->committedValue(violationId) == std::abs(Int(a) - Int(b)));
  RC_ASSERT(_solver->lowerBound(violationId) ==
            _solver->committedValue(violationId));
  RC_ASSERT(_solver->upperBound(violationId) ==
            _solver->committedValue(violationId));
}

RC_GTEST_FIXTURE_PROP(EqualViewConst, interval, (int a, int b)) {
  const Int size = 5;
  Int lb = Int(a) - size;
  Int ub = Int(a) + size;

  _solver->open();
  const VarViewId varId = _solver->makeIntVar(ub, lb, ub);
  const VarViewId violationId =
      _solver->makeIntView<EqualConst>(*_solver, varId, b);
  _solver->close();

  const Int violLb = _solver->lowerBound(violationId);
  const Int violUb = _solver->upperBound(violationId);

  for (Int val = lb; val <= ub; ++val) {
    _solver->beginMove();
    _solver->setValue(varId, val);
    _solver->endMove();
    _solver->beginProbe();
    _solver->query(violationId);
    _solver->endProbe();

    const Int actual = _solver->currentValue(violationId);
    const Int expected = std::abs(val - Int(b));

    RC_ASSERT(_solver->lowerBound(violationId) == violLb);
    RC_ASSERT(_solver->upperBound(violationId) == violUb);
    RC_ASSERT(actual == expected);
    RC_ASSERT(violLb <= actual);
    RC_ASSERT(violUb >= actual);
  }
}

}  // namespace atlantis::testing
