
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <limits>
#include <random>
#include <vector>

#include "../testHelper.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "invariants/absDiff.hpp"

using ::testing::AnyNumber;
using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Exactly;
using ::testing::Return;

namespace {

class AbsDiffTest : public InvariantTest {
 public:
  Int computeOutput(Timestamp ts, std::array<VarId, 2> inputs) {
    return computeOutput(engine->value(ts, inputs.at(0)),
                         engine->value(ts, inputs.at(1)));
  }

  Int computeOutput(std::array<Int, 2> inputs) {
    return computeOutput(inputs.at(0), inputs.at(1));
  }

  Int computeOutput(Timestamp ts, const VarId x, const VarId y) {
    return computeOutput(engine->value(ts, x), engine->value(ts, y));
  }

  Int computeOutput(const Int aVal, const Int bVal) {
    return std::abs(aVal - bVal);
  }
};

TEST_F(AbsDiffTest, Recompute) {
  const Int aLb = -1;
  const Int aUb = 0;
  const Int bLb = 0;
  const Int bUb = 1;
  EXPECT_TRUE(aLb <= aUb);
  EXPECT_TRUE(bLb <= bUb);

  engine->open();
  const VarId x = engine->makeIntVar(aUb, aLb, aUb);
  const VarId y = engine->makeIntVar(bUb, bLb, bUb);
  const VarId outputId =
      engine->makeIntVar(0, 0, std::max(aUb - bLb, bUb - aLb));
  AbsDiff& invariant = engine->makeInvariant<AbsDiff>(x, y, outputId);
  engine->close();

  for (Int aVal = aLb; aVal <= aUb; ++aVal) {
    for (Int bVal = bLb; bVal <= bUb; ++bVal) {
      engine->setValue(engine->currentTimestamp(), x, aVal);
      engine->setValue(engine->currentTimestamp(), y, bVal);

      const Int expectedOutput = computeOutput(aVal, bVal);
      invariant.recompute(engine->currentTimestamp(), *engine);
      EXPECT_EQ(expectedOutput,
                engine->value(engine->currentTimestamp(), outputId));
    }
  }
}

TEST_F(AbsDiffTest, NotifyInputChanged) {
  const Int lb = -50;
  const Int ub = -49;
  EXPECT_TRUE(lb <= ub);

  engine->open();
  std::array<VarId, 2> inputs{engine->makeIntVar(ub, lb, ub),
                              engine->makeIntVar(ub, lb, ub)};
  VarId outputId = engine->makeIntVar(0, 0, ub - lb);
  AbsDiff& invariant =
      engine->makeInvariant<AbsDiff>(inputs.at(0), inputs.at(1), outputId);
  engine->close();

  for (Int val = lb; val <= ub; ++val) {
    for (size_t i = 0; i < inputs.size(); ++i) {
      engine->setValue(engine->currentTimestamp(), inputs.at(i), val);
      const Int expectedOutput =
          computeOutput(engine->currentTimestamp(), inputs);

      invariant.notifyInputChanged(engine->currentTimestamp(), *engine,
                                   LocalId(i));
      EXPECT_EQ(expectedOutput,
                engine->value(engine->currentTimestamp(), outputId));
    }
  }
}

TEST_F(AbsDiffTest, NextInput) {
  const Int lb = 5;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  engine->open();
  const std::array<VarId, 2> inputs = {engine->makeIntVar(0, lb, ub),
                                       engine->makeIntVar(1, lb, ub)};
  const VarId outputId = engine->makeIntVar(0, 0, 2);
  const VarId minVarId = *std::min_element(inputs.begin(), inputs.end());
  ;
  const VarId maxVarId = *std::max_element(inputs.begin(), inputs.end());
  ;
  AbsDiff& invariant =
      engine->makeInvariant<AbsDiff>(inputs.at(0), inputs.at(1), outputId);
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

TEST_F(AbsDiffTest, NotifyCurrentInputChanged) {
  const Int lb = -10002;
  const Int ub = -10000;
  EXPECT_TRUE(lb <= ub);

  engine->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  const std::array<VarId, 2> inputs = {
      engine->makeIntVar(valueDist(gen), lb, ub),
      engine->makeIntVar(valueDist(gen), lb, ub)};
  const VarId outputId = engine->makeIntVar(0, 0, ub - lb);
  AbsDiff& invariant =
      engine->makeInvariant<AbsDiff>(inputs.at(0), inputs.at(1), outputId);
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
      EXPECT_EQ(engine->value(ts, outputId), computeOutput(ts, inputs));
    }
  }
}

TEST_F(AbsDiffTest, Commit) {
  const Int lb = -10;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);

  engine->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  std::array<size_t, 2> indices{0, 1};
  std::array<Int, 2> committedValues{valueDist(gen), valueDist(gen)};
  std::array<VarId, 2> inputs{
      engine->makeIntVar(committedValues.at(0), lb, ub),
      engine->makeIntVar(committedValues.at(1), lb, ub)};
  std::shuffle(indices.begin(), indices.end(), rng);

  VarId outputId = engine->makeIntVar(0, 0, 2);
  AbsDiff& invariant =
      engine->makeInvariant<AbsDiff>(inputs.at(0), inputs.at(1), outputId);
  engine->close();

  EXPECT_EQ(engine->value(engine->currentTimestamp(), outputId),
            computeOutput(engine->currentTimestamp(), inputs));

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
    const Int notifiedOutput = engine->value(ts, outputId);
    invariant.recompute(ts, *engine);

    ASSERT_EQ(notifiedOutput, engine->value(ts, outputId));

    engine->commitIf(ts, inputs.at(i));
    committedValues.at(i) = engine->value(ts, inputs.at(i));
    engine->commitIf(ts, outputId);

    invariant.commit(ts, *engine);
    invariant.recompute(ts + 1, *engine);
    ASSERT_EQ(notifiedOutput, engine->value(ts + 1, outputId));
  }
}

class MockAbsDiff : public AbsDiff {
 public:
  bool initialized = false;
  void init(Timestamp timestamp, Engine& engine) override {
    initialized = true;
    AbsDiff::init(timestamp, engine);
  }
  MockAbsDiff(VarId x, VarId y, VarId c) : AbsDiff(x, y, c) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return AbsDiff::recompute(timestamp, engine);
        });
    ON_CALL(*this, nextInput)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          return AbsDiff::nextInput(t, engine);
        });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          AbsDiff::notifyCurrentInputChanged(t, engine);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine, LocalId id) {
          AbsDiff::notifyInputChanged(t, engine, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp t, Engine& engine) {
      AbsDiff::commit(t, engine);
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
TEST_F(AbsDiffTest, EngineIntegration) {
  for (const auto [propMode, markingMode] : propMarkModes) {
    if (!engine->isOpen()) {
      engine->open();
    }
    const VarId x = engine->makeIntVar(-10, -100, 100);
    const VarId y = engine->makeIntVar(10, -100, 100);
    const VarId output = engine->makeIntVar(0, 0, 200);
    testNotifications<MockAbsDiff>(
        &engine->makeInvariant<MockAbsDiff>(x, y, output), propMode,
        markingMode, 3, x, 0, output);
  }
}

}  // namespace
