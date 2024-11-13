#include "../invariantTestHelper.hpp"
#include "atlantis/propagation/invariants/mod.hpp"

namespace atlantis::testing {

using namespace atlantis::propagation;

class ModTest : public InvariantTest {
 public:
  VarViewId numerator{NULL_ID};
  VarViewId denominator{NULL_ID};
  Int numeratorLb{-2};
  Int numeratorUb{2};
  Int denominatorLb{-2};
  Int denominatorUb{2};
  VarViewId outputVar{NULL_ID};

  std::uniform_int_distribution<Int> numeratorDist;
  std::uniform_int_distribution<Int> denominatorDist;

  Int computeOutput(Timestamp ts) {
    return computeOutput(_solver->value(ts, numerator),
                         _solver->value(ts, denominator));
  }

  Int computeOutput(bool committedValue = false) {
    return computeOutput(committedValue ? _solver->committedValue(numerator)
                                        : _solver->currentValue(numerator),
                         committedValue ? _solver->committedValue(denominator)
                                        : _solver->currentValue(denominator));
  }

  Int computeOutput(Int numerator, Int denominator) {
    return numerator % (denominator == 0 ? 1 : std::abs(denominator));
  }

  Mod& generate(bool denominatorZero = false) {
    numeratorDist =
        std::uniform_int_distribution<Int>(numeratorLb, numeratorUb);
    denominatorDist =
        std::uniform_int_distribution<Int>(denominatorLb, denominatorUb);

    if (!_solver->isOpen()) {
      _solver->open();
    }
    numerator = makeIntVar(numeratorLb, numeratorUb, numeratorDist);
    denominator =
        _solver->makeIntVar(denominatorZero ? 0 : denominatorDist(gen),
                            denominatorLb, denominatorUb);
    outputVar = _solver->makeIntVar(0, 0, 0);
    Mod& invariant = _solver->makeInvariant<Mod>(*_solver, outputVar, numerator,
                                                 denominator);
    _solver->close();
    return invariant;
  }
};

TEST_F(ModTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-20, -15}, {-5, 0}, {-2, 2}, {0, 5}, {15, 20}};
  auto& invariant = generate();

  for (const auto& [numLb, numUb] : boundVec) {
    EXPECT_LE(numLb, numUb);
    _solver->updateBounds(VarId(numerator), numLb, numUb, false);
    for (const auto& [denLb, denUb] : boundVec) {
      EXPECT_LE(denLb, denUb);
      _solver->updateBounds(VarId(denominator), denLb, denUb, false);
      _solver->open();
      invariant.updateBounds(false);
      _solver->close();
      for (Int numVal = numLb; numVal <= numUb; ++numVal) {
        _solver->setValue(_solver->currentTimestamp(), numerator, numVal);
        for (Int denVal = denLb; denVal <= denUb; ++denVal) {
          _solver->setValue(_solver->currentTimestamp(), denominator, denVal);
          invariant.recompute(_solver->currentTimestamp());
          const Int o = _solver->currentValue(outputVar);
          if (o < _solver->lowerBound(outputVar) ||
              _solver->upperBound(outputVar) < o) {
            ASSERT_GE(o, _solver->lowerBound(outputVar));
            ASSERT_LE(o, _solver->upperBound(outputVar));
          }
        }
      }
    }
  }
}

TEST_F(ModTest, Recompute) {
  generateState = GenerateState::LB;

  auto& invariant = generate();

  std::vector<VarViewId> inputVars{numerator, denominator};

  auto inputVals = makeValVector(inputVars);

  Timestamp ts = _solver->currentTimestamp();

  while (increaseNextVal(inputVars, inputVals) >= 0) {
    ++ts;
    setVarVals(ts, inputVars, inputVals);

    const Int expectedOutput = computeOutput(ts);
    invariant.recompute(ts);
    EXPECT_EQ(expectedOutput, _solver->value(ts, outputVar));
  }
}

TEST_F(ModTest, NotifyInputChanged) {
  generateState = GenerateState::LB;

  auto& invariant = generate();

  std::vector<VarViewId> inputVars{numerator, denominator};

  auto inputVals = makeValVector(inputVars);

  Timestamp ts = _solver->currentTimestamp();

  while (increaseNextVal(inputVars, inputVals) >= 0) {
    ++ts;
    setVarVals(ts, inputVars, inputVals);

    const Int expectedOutput = computeOutput(ts);
    notifyInputsChanged(ts, invariant, inputVars);
    EXPECT_EQ(expectedOutput, _solver->value(ts, outputVar));
  }
}

TEST_F(ModTest, NextInput) {
  auto& invariant = generate();

  std::vector<VarViewId> inputVars{numerator, denominator};

  expectNextInput(inputVars, invariant);
}

TEST_F(ModTest, NotifyCurrentInputChanged) {
  auto& invariant = generate();

  std::vector<VarViewId> inputVars{numerator, denominator};

  for (Timestamp ts = _solver->currentTimestamp() + 1;
       ts < _solver->currentTimestamp() + 4; ++ts) {
    for (const VarViewId& varId : inputVars) {
      EXPECT_EQ(invariant.nextInput(ts), varId);
      const Int oldVal = _solver->value(ts, varId);
      do {
        _solver->setValue(
            ts, varId,
            varId == numerator ? numeratorDist(gen) : denominatorDist(gen));
      } while (_solver->value(ts, varId) == oldVal);
      invariant.notifyCurrentInputChanged(ts);
      EXPECT_EQ(_solver->value(ts, outputVar), computeOutput(ts));
    }
  }
}

TEST_F(ModTest, Commit) {
  auto& invariant = generate();

  std::vector<VarViewId> inputVars{numerator, denominator};

  std::vector<size_t> indices{0, 1};
  std::shuffle(indices.begin(), indices.end(), rng);

  std::vector<Int> committedValues{_solver->committedValue(numerator),
                                   _solver->committedValue(denominator)};

  EXPECT_EQ(_solver->currentValue(outputVar), computeOutput());

  for (const size_t i : indices) {
    Timestamp ts = _solver->currentTimestamp() + Timestamp(1 + i);
    for (size_t j = 0; j < inputVars.size(); ++j) {
      // Check that we do not accidentally commit:
      ASSERT_EQ(_solver->committedValue(inputVars.at(j)),
                committedValues.at(j));
    }

    const Int oldVal = committedValues.at(i);
    do {
      _solver->setValue(ts, inputVars.at(i),
                        i == 0 ? numeratorDist(gen) : denominatorDist(gen));
    } while (oldVal == _solver->value(ts, inputVars.at(i)));

    // notify changes
    invariant.notifyInputChanged(ts, LocalId(i));

    // incremental value
    const Int notifiedOutput = _solver->value(ts, outputVar);
    invariant.recompute(ts);

    ASSERT_EQ(notifiedOutput, _solver->value(ts, outputVar));

    _solver->commitIf(ts, VarId(inputVars.at(i)));
    committedValues.at(i) = _solver->value(ts, VarId(inputVars.at(i)));
    _solver->commitIf(ts, VarId(outputVar));

    invariant.commit(ts);
    invariant.recompute(ts + 1);
    ASSERT_EQ(notifiedOutput, _solver->value(ts + 1, outputVar));
  }
}

TEST_F(ModTest, ZeroDenominator) {
  numeratorLb = 10;
  numeratorUb = 10;

  for (const auto& [denLb, denUb, expected] : std::vector<std::array<Int, 3>>{
           {-100, 0, 0}, {-50, 50, 0}, {0, 100, 0}}) {
    EXPECT_LE(denLb, denUb);
    EXPECT_TRUE(denLb != 0 || denUb != 0);
    denominatorLb = denLb;
    denominatorUb = denUb;

    for (size_t method = 0; method < 2; ++method) {
      auto& invariant = generate(true);

      EXPECT_EQ(expected, computeOutput());
      if (method == 0) {
        invariant.recompute(_solver->currentTimestamp());
      } else {
        invariant.notifyInputChanged(_solver->currentTimestamp(), LocalId(1));
      }
      EXPECT_EQ(expected, _solver->currentValue(outputVar));
    }
  }
}

RC_GTEST_FIXTURE_PROP(ModTest, rapidcheck, ()) {
  _solver->open();

  const Int n1 = *rc::gen::arbitrary<Int>();
  const Int n2 = *rc::gen::arbitrary<Int>();
  numeratorLb = std::min(n1, n2);
  numeratorUb = std::max(n1, n2);

  const Int d1 = *rc::gen::arbitrary<Int>();
  const Int d2 = *rc::gen::suchThat(rc::gen::arbitrary<Int>(),
                                    [&](Int d) { return d1 != 0 || d != 0; });

  denominatorLb = std::min(d1, d2);
  denominatorUb = std::max(d1, d2);

  generate();

  const size_t numCommits = 3;
  const size_t numProbes = 3;

  for (size_t c = 0; c < numCommits; ++c) {
    RC_ASSERT(_solver->committedValue(outputVar) == computeOutput(true));

    for (size_t p = 0; p <= numProbes; ++p) {
      _solver->beginMove();
      if (randBool()) {
        _solver->setValue(numerator, numeratorDist(gen));
      }
      if (randBool()) {
        _solver->setValue(denominator, denominatorDist(gen));
      }

      _solver->endMove();

      if (p == numProbes) {
        _solver->beginCommit();
      } else {
        _solver->beginProbe();
      }
      _solver->query(outputVar);
      if (p == numProbes) {
        _solver->endCommit();
      } else {
        _solver->endProbe();
      }
      RC_ASSERT(_solver->currentValue(outputVar) == computeOutput());
    }
    RC_ASSERT(_solver->committedValue(outputVar) == computeOutput(true));
  }
}

class MockMod : public Mod {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    Mod::registerVars();
  }
  explicit MockMod(SolverBase& solver, VarViewId output, VarViewId numerator,
                   VarViewId denominator)
      : Mod(solver, output, numerator, denominator) {
    EXPECT_TRUE(output.isVar());

    ON_CALL(*this, recompute).WillByDefault([this](Timestamp timestamp) {
      return Mod::recompute(timestamp);
    });
    ON_CALL(*this, nextInput).WillByDefault([this](Timestamp timestamp) {
      return Mod::nextInput(timestamp);
    });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp timestamp) {
          Mod::notifyCurrentInputChanged(timestamp);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp timestamp, LocalId id) {
          Mod::notifyInputChanged(timestamp, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp timestamp) {
      Mod::commit(timestamp);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp), (override));
  MOCK_METHOD(VarViewId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(ModTest, SolverIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!_solver->isOpen()) {
      _solver->open();
    }
    const VarViewId numerator = _solver->makeIntVar(-10, -100, 100);
    const VarViewId denominator = _solver->makeIntVar(10, -100, 100);
    const VarViewId output = _solver->makeIntVar(0, 0, 200);
    testNotifications<MockMod>(
        &_solver->makeInvariant<MockMod>(*_solver, output, numerator,
                                         denominator),
        {propMode, markingMode, 3, numerator, 0, output});
  }
}

}  // namespace atlantis::testing
