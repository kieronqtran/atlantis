#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

#include "../testHelper.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "invariants/mod.hpp"

using ::testing::AnyNumber;
using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Exactly;

namespace {

class ModTest : public InvariantTest {
 public:
  Int computeOutput(Timestamp ts, std::array<VarId, 2> inputs) {
    return computeOutput(ts, inputs.at(0), inputs.at(1));
  }

  Int computeOutput(Timestamp ts, const VarId x, const VarId y) {
    Int denominator = engine->value(ts, y);
    if (denominator == 0) {
      denominator = engine->upperBound(y) > 0 ? 1 : -1;
    }
    return engine->value(ts, x) % std::abs(denominator);
  }
};

TEST_F(ModTest, Examples) {
  std::vector<std::array<Int, 3>> data{
      {7, 4, 3}, {-7, 4, -3}, {7, -4, 3}, {-7, -4, -3}};

  Int xLb = std::numeric_limits<Int>::max();
  Int xUb = std::numeric_limits<Int>::min();
  Int yLb = std::numeric_limits<Int>::max();
  Int yUb = std::numeric_limits<Int>::min();
  Int outputLb = std::numeric_limits<Int>::max();
  Int outputUb = std::numeric_limits<Int>::min();

  for (const auto& [xVal, yVal, outputVal] : data) {
    xLb = std::min(xLb, xVal);
    xUb = std::max(xUb, xVal);
    yLb = std::min(yLb, yVal);
    yUb = std::max(yUb, yVal);
    outputLb = std::min(outputLb, outputVal);
    outputUb = std::max(outputUb, outputVal);
  }
  EXPECT_TRUE(xLb <= xUb);
  EXPECT_TRUE(yLb <= yUb);
  EXPECT_TRUE(yLb != 0 || yUb != 0);

  engine->open();
  const VarId x = engine->makeIntVar(xUb, xLb, xUb);
  const VarId y = engine->makeIntVar(yUb, yLb, yUb);
  const VarId outputId = engine->makeIntVar(0, outputLb, outputUb);
  Mod& invariant = engine->makeInvariant<Mod>(*engine, outputId, x, y);
  engine->close();

  for (size_t i = 0; i < data.size(); ++i) {
    Timestamp ts = engine->currentTimestamp() + Timestamp(i + 1);
    const Int xVal = data.at(i).at(0);
    const Int yVal = data.at(i).at(1);
    const Int expectedOutput = data.at(i).at(2);

    engine->setValue(ts, x, xVal);
    engine->setValue(ts, y, yVal);

    invariant.recompute(ts);

    EXPECT_EQ(engine->value(ts, outputId), expectedOutput);
    EXPECT_EQ(computeOutput(ts, x, y), expectedOutput);
  }
}

TEST_F(ModTest, UpdateBounds) {
  std::vector<std::pair<Int, Int>> boundVec{
      {-20, -15}, {-5, 0}, {-2, 2}, {0, 5}, {15, 20}};
  engine->open();
  const VarId x = engine->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarId y = engine->makeIntVar(
      boundVec.front().first, boundVec.front().first, boundVec.front().second);
  const VarId outputId = engine->makeIntVar(0, 0, 2);
  Mod& invariant = engine->makeInvariant<Mod>(*engine, outputId, x, y);
  engine->close();

  for (const auto& [xLb, xUb] : boundVec) {
    EXPECT_TRUE(xLb <= xUb);
    engine->updateBounds(x, xLb, xUb, false);
    for (const auto& [yLb, yUb] : boundVec) {
      EXPECT_TRUE(yLb <= yUb);
      engine->updateBounds(y, yLb, yUb, false);
      engine->open();
      invariant.updateBounds();
      engine->close();
      for (Int xVal = xLb; xVal <= xUb; ++xVal) {
        engine->setValue(engine->currentTimestamp(), x, xVal);
        for (Int yVal = yLb; yVal <= yUb; ++yVal) {
          engine->setValue(engine->currentTimestamp(), y, yVal);
          invariant.recompute(engine->currentTimestamp());
          const Int o = engine->value(engine->currentTimestamp(), outputId);
          if (o < engine->lowerBound(outputId) ||
              engine->upperBound(outputId) < o) {
            ASSERT_GE(o, engine->lowerBound(outputId));
            ASSERT_LE(o, engine->upperBound(outputId));
          }
        }
      }
    }
  }
}

TEST_F(ModTest, Recompute) {
  const Int xLb = -1;
  const Int xUb = 0;
  const Int yLb = 0;
  const Int yUb = 1;
  const Int outputLb = -1;
  const Int outputUb = 0;

  EXPECT_TRUE(xLb <= xUb);
  EXPECT_TRUE(yLb <= yUb);
  EXPECT_TRUE(yLb != 0 || yUb != 0);

  engine->open();
  const VarId x = engine->makeIntVar(xUb, xLb, xUb);
  const VarId y = engine->makeIntVar(yUb, yLb, yUb);
  const VarId outputId = engine->makeIntVar(outputLb, outputLb, outputUb);
  Mod& invariant = engine->makeInvariant<Mod>(*engine, outputId, x, y);
  engine->close();

  for (Int xVal = xLb; xVal <= xUb; ++xVal) {
    for (Int yVal = yLb; yVal <= yUb; ++yVal) {
      engine->setValue(engine->currentTimestamp(), x, xVal);
      engine->setValue(engine->currentTimestamp(), y, yVal);

      const Int expectedOutput =
          computeOutput(engine->currentTimestamp(), x, y);
      invariant.recompute(engine->currentTimestamp());
      EXPECT_EQ(expectedOutput,
                engine->value(engine->currentTimestamp(), outputId));
    }
  }
}

TEST_F(ModTest, NotifyInputChanged) {
  const Int lb = -50;
  const Int ub = -49;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(lb != 0 || ub != 0);

  engine->open();
  std::array<VarId, 2> inputs{engine->makeIntVar(ub, lb, ub),
                              engine->makeIntVar(ub, lb, ub)};
  VarId outputId = engine->makeIntVar(0, 0, ub - lb);
  Mod& invariant =
      engine->makeInvariant<Mod>(*engine, outputId, inputs.at(0), inputs.at(1));
  engine->close();

  Timestamp ts = engine->currentTimestamp();

  for (Int val = lb; val <= ub; ++val) {
    ++ts;
    for (size_t i = 0; i < inputs.size(); ++i) {
      engine->setValue(engine->currentTimestamp(), inputs.at(i), val);
      const Int expectedOutput =
          computeOutput(engine->currentTimestamp(), inputs);

      invariant.notifyInputChanged(engine->currentTimestamp(), LocalId(i));
      EXPECT_EQ(expectedOutput,
                engine->value(engine->currentTimestamp(), outputId));
    }
  }
}

TEST_F(ModTest, NextInput) {
  const Int lb = 5;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(lb != 0 || ub != 0);

  engine->open();
  const std::array<VarId, 2> inputs = {engine->makeIntVar(lb, lb, ub),
                                       engine->makeIntVar(ub, lb, ub)};
  const VarId outputId = engine->makeIntVar(0, 0, 2);
  const VarId minVarId = *std::min_element(inputs.begin(), inputs.end());
  const VarId maxVarId = *std::max_element(inputs.begin(), inputs.end());
  Mod& invariant =
      engine->makeInvariant<Mod>(*engine, outputId, inputs.at(0), inputs.at(1));
  engine->close();

  for (Timestamp ts = engine->currentTimestamp() + 1;
       ts < engine->currentTimestamp() + 4; ++ts) {
    std::vector<bool> notified(maxVarId + 1, false);
    for (size_t i = 0; i < inputs.size(); ++i) {
      const VarId varId = invariant.nextInput(ts);
      EXPECT_NE(varId, NULL_ID);
      EXPECT_TRUE(minVarId <= varId);
      EXPECT_TRUE(varId <= maxVarId);
      EXPECT_FALSE(notified.at(varId));
      notified[varId] = true;
    }
    EXPECT_EQ(invariant.nextInput(ts), NULL_ID);
    for (size_t varId = minVarId; varId <= maxVarId; ++varId) {
      EXPECT_TRUE(notified.at(varId));
    }
  }
}

TEST_F(ModTest, NotifyCurrentInputChanged) {
  const Int lb = -10002;
  const Int ub = -10000;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(lb != 0 || ub != 0);

  engine->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  const std::array<VarId, 2> inputs = {
      engine->makeIntVar(valueDist(gen), lb, ub),
      engine->makeIntVar(valueDist(gen), lb, ub)};
  const VarId outputId = engine->makeIntVar(0, 0, ub - lb);
  Mod& invariant =
      engine->makeInvariant<Mod>(*engine, outputId, inputs.at(0), inputs.at(1));
  engine->close();

  for (Timestamp ts = engine->currentTimestamp() + 1;
       ts < engine->currentTimestamp() + 4; ++ts) {
    for (const VarId varId : inputs) {
      EXPECT_EQ(invariant.nextInput(ts), varId);
      const Int oldVal = engine->value(ts, varId);
      do {
        engine->setValue(ts, varId, valueDist(gen));
      } while (engine->value(ts, varId) == oldVal);
      invariant.notifyCurrentInputChanged(ts);
      EXPECT_EQ(engine->value(ts, outputId), computeOutput(ts, inputs));
    }
  }
}

TEST_F(ModTest, Commit) {
  const Int lb = -10;
  const Int ub = 10;
  EXPECT_TRUE(lb <= ub);
  EXPECT_TRUE(lb != 0 || ub != 0);

  engine->open();
  std::uniform_int_distribution<Int> valueDist(lb, ub);
  std::array<size_t, 2> indices{0, 1};
  std::array<Int, 2> committedValues{valueDist(gen), valueDist(gen)};
  std::array<VarId, 2> inputs{
      engine->makeIntVar(committedValues.at(0), lb, ub),
      engine->makeIntVar(committedValues.at(1), lb, ub)};
  std::shuffle(indices.begin(), indices.end(), rng);

  VarId outputId = engine->makeIntVar(0, 0, 2);
  Mod& invariant =
      engine->makeInvariant<Mod>(*engine, outputId, inputs.at(0), inputs.at(1));
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
    invariant.notifyInputChanged(ts, LocalId(i));

    // incremental value
    const Int notifiedOutput = engine->value(ts, outputId);
    invariant.recompute(ts);

    ASSERT_EQ(notifiedOutput, engine->value(ts, outputId));

    engine->commitIf(ts, inputs.at(i));
    committedValues.at(i) = engine->value(ts, inputs.at(i));
    engine->commitIf(ts, outputId);

    invariant.commit(ts);
    invariant.recompute(ts + 1);
    ASSERT_EQ(notifiedOutput, engine->value(ts + 1, outputId));
  }
}

TEST_F(ModTest, ZeroDenominator) {
  const Int xVal = 10;
  const Int outputLb = std::numeric_limits<Int>::min();
  const Int outputUb = std::numeric_limits<Int>::max();
  for (const auto& [yLb, yUb, expected] : std::vector<std::array<Int, 3>>{
           {-100, 0, 0}, {-50, 50, 0}, {0, 100, 0}}) {
    EXPECT_TRUE(yLb <= yUb);
    EXPECT_TRUE(yLb != 0 || yUb != 0);

    for (size_t method = 0; method < 2; ++method) {
      engine->open();
      const VarId x = engine->makeIntVar(xVal, xVal, xVal);
      const VarId y = engine->makeIntVar(0, yLb, yUb);
      const VarId outputId = engine->makeIntVar(0, outputLb, outputUb);
      Mod& invariant = engine->makeInvariant<Mod>(*engine, outputId, x, y);
      engine->close();

      EXPECT_EQ(expected, computeOutput(engine->currentTimestamp(), x, y));
      if (method == 0) {
        invariant.recompute(engine->currentTimestamp());
      } else {
        invariant.notifyInputChanged(engine->currentTimestamp(), LocalId(1));
      }
      EXPECT_EQ(expected, engine->value(engine->currentTimestamp(), outputId));
    }
  }
}

class MockMod : public Mod {
 public:
  bool registered = false;
  void registerVars() override {
    registered = true;
    Mod::registerVars();
  }
  explicit MockMod(Engine& engine, VarId output, VarId x, VarId y)
      : Mod(engine, output, x, y) {
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
  MOCK_METHOD(VarId, nextInput, (Timestamp), (override));
  MOCK_METHOD(void, notifyCurrentInputChanged, (Timestamp), (override));
  MOCK_METHOD(void, notifyInputChanged, (Timestamp, LocalId), (override));
  MOCK_METHOD(void, commit, (Timestamp), (override));
};
TEST_F(ModTest, EngineIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!engine->isOpen()) {
      engine->open();
    }
    const VarId x = engine->makeIntVar(-10, -100, 100);
    const VarId y = engine->makeIntVar(10, -100, 100);
    const VarId output = engine->makeIntVar(0, 0, 200);
    testNotifications<MockMod>(
        &engine->makeInvariant<MockMod>(*engine, output, x, y),
        {propMode, markingMode, 3, x, 0, output});
  }
}

}  // namespace
