#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <random>
#include <vector>

#include "../testHelper.hpp"
#include "constraints/equal.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"

using ::testing::AtLeast;
using ::testing::Return;

namespace {

class EqualTest : public InvariantTest {
 public:
  Int computeViolation(const Timestamp ts,
                       const std::array<const VarId, 2>& inputs) {
    return computeViolation(engine->value(ts, inputs.at(0)),
                            engine->value(ts, inputs.at(1)));
  }

  Int computeViolation(const std::array<const Int, 2>& inputs) {
    return computeViolation(inputs.at(0), inputs.at(1));
  }

  Int computeViolation(const Timestamp ts, const VarId a, const VarId b) {
    return computeViolation(engine->value(ts, a), engine->value(ts, b));
  }

  Int computeViolation(const Int aVal, const Int bVal) {
    return std::abs(aVal - bVal);
  }
};

/**
 *  Testing constructor
 */

TEST_F(EqualTest, Recompute) {
  const Int aLb = -100;
  const Int aUb = 25;
  const Int bLb = -25;
  const Int bUb = 50;

  EXPECT_TRUE(aLb <= aUb);
  EXPECT_TRUE(bLb <= bUb);
  engine->open();
  const std::array<const VarId, 2> inputs{engine->makeIntVar(aUb, aLb, aUb),
                                          engine->makeIntVar(bUb, bLb, bUb)};
  const VarId violationId =
      engine->makeIntVar(0, 0, std::max(aUb - bLb, bUb - aLb));
  Equal& invariant =
      engine->makeConstraint<Equal>(violationId, inputs.at(0), inputs.at(1));
  engine->close();

  for (Int aVal = aLb; aVal <= aUb; ++aVal) {
    for (Int bVal = bLb; bVal <= bUb; ++bVal) {
      engine->setValue(engine->currentTimestamp(), inputs.at(0), aVal);
      engine->setValue(engine->currentTimestamp(), inputs.at(1), bVal);

      const Int expectedViolation = computeViolation(aVal, bVal);
      invariant.recompute(engine->currentTimestamp(), *engine);
      EXPECT_EQ(expectedViolation,
                engine->value(engine->currentTimestamp(), violationId));
    }
  }
}

TEST_F(EqualTest, NotifyInputChanged) {
  const Int lb = -50;
  const Int ub = 50;
  EXPECT_TRUE(lb <= ub);

  engine->open();
  const std::array<const VarId, 2> inputs{engine->makeIntVar(ub, lb, ub),
                                          engine->makeIntVar(ub, lb, ub)};
  const VarId violationId = engine->makeIntVar(0, 0, ub - lb);
  Equal& invariant =
      engine->makeConstraint<Equal>(violationId, inputs.at(0), inputs.at(1));
  engine->close();

  for (Int val = lb; val <= ub; ++val) {
    for (size_t i = 0; i < inputs.size(); ++i) {
      engine->setValue(engine->currentTimestamp(), inputs.at(i), val);
      const Int expectedViolation =
          computeViolation(engine->currentTimestamp(), inputs);

      invariant.notifyInputChanged(engine->currentTimestamp(), *engine,
                                   LocalId(i));
      EXPECT_EQ(expectedViolation,
                engine->value(engine->currentTimestamp(), violationId));
    }
  }
}

TEST_F(EqualTest, NextInput) {
  const Int lb = -10;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  engine->open();
  const std::array<const VarId, 2> inputs = {engine->makeIntVar(0, lb, ub),
                                             engine->makeIntVar(1, lb, ub)};
  const VarId violationId = engine->makeIntVar(0, 0, 2);
  const VarId minVarId = *std::min_element(inputs.begin(), inputs.end());
  const VarId maxVarId = *std::max_element(inputs.begin(), inputs.end());
  Equal& invariant =
      engine->makeConstraint<Equal>(violationId, inputs.at(0), inputs.at(1));
  engine->close();

  for (Timestamp ts = engine->currentTimestamp() + 1;
       ts < engine->currentTimestamp() + 4; ++ts) {
    std::vector<bool> notified(maxVarId + 1, false);
    for (size_t i = 0; i < inputs.size(); ++i) {
      const VarId varId = invariant.nextInput(ts, *engine);
      EXPECT_NE(varId, NULL_ID);
      EXPECT_TRUE(minVarId <= varId);
      EXPECT_TRUE(varId <= maxVarId);
      EXPECT_FALSE(notified.at(varId));
      notified[varId] = true;
    }
    EXPECT_EQ(invariant.nextInput(ts, *engine), NULL_ID);
    for (size_t varId = minVarId; varId <= maxVarId; ++varId) {
      EXPECT_TRUE(notified.at(varId));
    }
  }
}

TEST_F(EqualTest, NotifyCurrentInputChanged) {
  const Int lb = -10;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  engine->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  const std::array<const VarId, 2> inputs = {
      engine->makeIntVar(valueDist(gen), lb, ub),
      engine->makeIntVar(valueDist(gen), lb, ub)};
  const VarId violationId = engine->makeIntVar(0, 0, ub - lb);
  Equal& invariant =
      engine->makeConstraint<Equal>(violationId, inputs.at(0), inputs.at(1));
  engine->close();

  for (Timestamp ts = engine->currentTimestamp() + 1;
       ts < engine->currentTimestamp() + 4; ++ts) {
    for (const VarId varId : inputs) {
      EXPECT_EQ(invariant.nextInput(ts, *engine), varId);
      const Int oldVal = engine->value(ts, varId);
      do {
        engine->setValue(ts, varId, valueDist(gen));
      } while (engine->value(ts, varId) == oldVal);
      invariant.notifyCurrentInputChanged(ts, *engine);
      EXPECT_EQ(engine->value(ts, violationId), computeViolation(ts, inputs));
    }
  }
}

TEST_F(EqualTest, Commit) {
  const Int lb = -10;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  std::uniform_int_distribution<Int> valueDist(lb, ub);
  std::array<size_t, 2> indices{0, 1};
  std::array<Int, 2> committedValues{valueDist(gen), valueDist(gen)};
  std::shuffle(indices.begin(), indices.end(), rng);

  engine->open();
  const std::array<const VarId, 2> inputs{
      engine->makeIntVar(committedValues.at(0), lb, ub),
      engine->makeIntVar(committedValues.at(1), lb, ub)};

  const VarId violationId = engine->makeIntVar(0, 0, 2);
  Equal& invariant =
      engine->makeConstraint<Equal>(violationId, inputs.at(0), inputs.at(1));
  engine->close();

  EXPECT_EQ(engine->value(engine->currentTimestamp(), violationId),
            computeViolation(engine->currentTimestamp(), inputs));

  for (const size_t i : indices) {
    Timestamp ts = engine->currentTimestamp() + Timestamp(1 + i);
    for (size_t j = 0; j < inputs.size(); ++j) {
      // Check that we do not accidentally commit:
      ASSERT_EQ(engine->committedValue(inputs.at(j)), committedValues.at(j));
    }

    const Int oldVal = committedValues.at(i);
    do {
      engine->setValue(ts, inputs.at(i), valueDist(gen));
    } while (oldVal == engine->value(ts, inputs.at(i)));

    // notify changes
    invariant.notifyInputChanged(ts, *engine, LocalId(i));

    // incremental value
    const Int notifiedViolation = engine->value(ts, violationId);
    invariant.recompute(ts, *engine);

    ASSERT_EQ(notifiedViolation, engine->value(ts, violationId));

    engine->commitIf(ts, inputs.at(i));
    committedValues.at(i) = engine->value(ts, inputs.at(i));
    engine->commitIf(ts, violationId);

    invariant.commit(ts, *engine);
    invariant.recompute(ts + 1, *engine);
    ASSERT_EQ(notifiedViolation, engine->value(ts + 1, violationId));
  }
}

class MockEqual : public Equal {
 public:
  bool initialized = false;
  void init(Timestamp timestamp, Engine& engine) override {
    initialized = true;
    Equal::init(timestamp, engine);
  }
  MockEqual(VarId violationId, VarId a, VarId b) : Equal(violationId, a, b) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return Equal::recompute(timestamp, engine);
        });
    ON_CALL(*this, nextInput)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          return Equal::nextInput(t, engine);
        });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          Equal::notifyCurrentInputChanged(t, engine);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine, LocalId id) {
          Equal::notifyInputChanged(t, engine, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp t, Engine& engine) {
      Equal::commit(t, engine);
    });
  }
  MOCK_METHOD(void, recompute, (Timestamp timestamp, Engine& engine),
              (override));
  MOCK_METHOD(VarId, nextInput, (Timestamp, Engine&), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp, Engine& engine),
              (override));
  MOCK_METHOD(void, notifyInputChanged,
              (Timestamp t, Engine& engine, LocalId id), (override));
  MOCK_METHOD(void, commit, (Timestamp timestamp, Engine& engine), (override));
};
TEST_F(EqualTest, EngineIntegration) {
  for (const auto [propMode, markingMode] : propMarkModes) {
    if (!engine->isOpen()) {
      engine->open();
    }
    const VarId a = engine->makeIntVar(5, -100, 100);
    const VarId b = engine->makeIntVar(0, -100, 100);
    const VarId viol = engine->makeIntVar(0, 0, 200);
    testNotifications<MockEqual>(&engine->makeInvariant<MockEqual>(viol, a, b),
                                 propMode, markingMode, 3, a, 1, viol);
  }
}
}  // namespace
