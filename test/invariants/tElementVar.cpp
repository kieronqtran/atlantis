
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <limits>
#include <random>
#include <vector>

#include "../testHelper.hpp"
#include "core/propagationEngine.hpp"
#include "core/types.hpp"
#include "invariants/elementVar.hpp"

using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::Return;

namespace {

class ElementVarTest : public InvariantTest {
 protected:
  const size_t numValues = 100;
  const Int inputLb = std::numeric_limits<Int>::min();
  const Int inputUb = std::numeric_limits<Int>::max();
  std::vector<Int> offsets{-10, 0, 10};
  std::vector<VarId> inputs;
  std::uniform_int_distribution<Int> inputDist;

 public:
  void SetUp() override {
    InvariantTest::SetUp();
    inputs.resize(numValues, NULL_ID);
    inputDist = std::uniform_int_distribution<Int>(inputLb, inputUb);
  }

  void TearDown() override {
    InvariantTest::TearDown();
    inputs.clear();
  }

  size_t zeroBasedIndex(const Int indexVal, const Int offset) {
    EXPECT_LE(offset, indexVal);
    EXPECT_LT(indexVal - offset, static_cast<Int>(numValues));
    return indexVal - offset;
  }

  VarId getInput(const Int indexVal, const Int offset) {
    return inputs.at(zeroBasedIndex(indexVal, offset));
  }

  Int computeOutput(const Timestamp ts, const VarId index, const Int offset) {
    return computeOutput(ts, engine->value(ts, index), offset);
  }

  Int computeOutput(const Timestamp ts, const Int indexVal, const Int offset) {
    return engine->value(ts, getInput(indexVal, offset));
  }
};

TEST_F(ElementVarTest, UpdateBounds) {
  EXPECT_TRUE(inputLb <= inputUb);
  for (const Int offset : offsets) {
    const Int indexLb = offset;
    const Int indexUb = numValues - 1 + offset;
    EXPECT_TRUE(indexLb <= indexUb);

    std::uniform_int_distribution<Int> indexDist(indexLb, indexUb);

    engine->open();
    const VarId index = engine->makeIntVar(indexDist(gen), indexLb, indexUb);
    for (size_t i = 0; i < inputs.size(); ++i) {
      inputs.at(i) = engine->makeIntVar(inputDist(gen), inputLb, inputUb);
    }
    const VarId outputId = engine->makeIntVar(inputLb, inputLb, inputUb);
    ElementVar& invariant =
        engine->makeInvariant<ElementVar>(outputId, index, inputs, offset);
    engine->close();

    for (Int minIndex = indexLb; minIndex <= indexUb; ++minIndex) {
      for (Int maxIndex = indexUb; maxIndex >= minIndex; --maxIndex) {
        engine->updateBounds(index, minIndex, maxIndex, false);
        invariant.updateBounds(*engine);
        Int minVal = std::numeric_limits<Int>::max();
        Int maxVal = std::numeric_limits<Int>::min();
        for (Int indexVal = minIndex; indexVal <= maxIndex; ++indexVal) {
          minVal =
              std::min(minVal, engine->lowerBound(getInput(indexVal, offset)));
          maxVal =
              std::max(maxVal, engine->upperBound(getInput(indexVal, offset)));
        }
        EXPECT_EQ(minVal, engine->lowerBound(outputId));
        EXPECT_EQ(maxVal, engine->upperBound(outputId));
      }
    }
  }
}

TEST_F(ElementVarTest, Recompute) {
  EXPECT_TRUE(inputLb <= inputUb);
  for (const Int offset : offsets) {
    const Int indexLb = offset;
    const Int indexUb = numValues - 1 + offset;
    EXPECT_TRUE(indexLb <= indexUb);

    std::uniform_int_distribution<Int> indexDist(indexLb, indexUb);

    engine->open();
    const VarId index = engine->makeIntVar(indexDist(gen), indexLb, indexUb);
    for (size_t i = 0; i < inputs.size(); ++i) {
      inputs.at(i) = engine->makeIntVar(inputDist(gen), inputLb, inputUb);
    }
    const VarId outputId = engine->makeIntVar(inputLb, inputLb, inputUb);
    ElementVar& invariant =
        engine->makeInvariant<ElementVar>(outputId, index, inputs, offset);
    engine->close();

    for (Int indexVal = indexLb; indexVal <= indexUb; ++indexVal) {
      engine->setValue(engine->currentTimestamp(), index, indexVal);
      EXPECT_EQ(engine->value(engine->currentTimestamp(), index), indexVal);

      const Int expectedOutput =
          computeOutput(engine->currentTimestamp(), index, offset);
      invariant.recompute(engine->currentTimestamp(), *engine);
      EXPECT_EQ(engine->value(engine->currentTimestamp(), index), indexVal);

      EXPECT_EQ(expectedOutput,
                engine->value(engine->currentTimestamp(), outputId));
    }
  }
}

TEST_F(ElementVarTest, NotifyInputChanged) {
  EXPECT_TRUE(inputLb <= inputUb);
  for (const Int offset : offsets) {
    const Int indexLb = offset;
    const Int indexUb = numValues - 1 + offset;
    EXPECT_TRUE(indexLb <= indexUb);

    std::uniform_int_distribution<Int> indexDist(indexLb, indexUb);

    engine->open();
    const VarId index = engine->makeIntVar(indexDist(gen), indexLb, indexUb);
    for (size_t i = 0; i < inputs.size(); ++i) {
      inputs.at(i) = engine->makeIntVar(inputDist(gen), inputLb, inputUb);
    }
    const VarId outputId = engine->makeIntVar(inputLb, inputLb, inputUb);
    ElementVar& invariant =
        engine->makeInvariant<ElementVar>(outputId, index, inputs, offset);
    engine->close();

    for (Int indexVal = indexLb; indexVal <= indexUb; ++indexVal) {
      engine->setValue(engine->currentTimestamp(), index, indexVal);
      const Int expectedOutput =
          computeOutput(engine->currentTimestamp(), index, offset);

      invariant.notifyInputChanged(engine->currentTimestamp(), *engine,
                                   LocalId(0));
      EXPECT_EQ(expectedOutput,
                engine->value(engine->currentTimestamp(), outputId));
    }
  }
}

TEST_F(ElementVarTest, NextInput) {
  EXPECT_TRUE(inputLb <= inputUb);
  for (const Int offset : offsets) {
    const Int indexLb = offset;
    const Int indexUb = numValues - 1 + offset;
    EXPECT_TRUE(indexLb <= indexUb);

    std::uniform_int_distribution<Int> indexDist(indexLb, indexUb);

    engine->open();
    const VarId index = engine->makeIntVar(indexDist(gen), indexLb, indexUb);
    for (size_t i = 0; i < inputs.size(); ++i) {
      inputs.at(i) = engine->makeIntVar(inputDist(gen), inputLb, inputUb);
    }
    const VarId outputId = engine->makeIntVar(inputLb, inputLb, inputUb);
    ElementVar& invariant =
        engine->makeInvariant<ElementVar>(outputId, index, inputs, offset);
    engine->close();

    for (Timestamp ts = engine->currentTimestamp() + 1;
         ts < engine->currentTimestamp() + 4; ++ts) {
      EXPECT_EQ(invariant.nextInput(ts, *engine), index);
      EXPECT_EQ(invariant.nextInput(ts, *engine),
                getInput(engine->value(ts, index), offset));
      EXPECT_EQ(invariant.nextInput(ts, *engine), NULL_ID);
    }
  }
}

TEST_F(ElementVarTest, NotifyCurrentInputChanged) {
  EXPECT_TRUE(inputLb <= inputUb);
  Timestamp t0 = engine->currentTimestamp() +
                 (numValues * static_cast<Int>(offsets.size())) + 1;
  for (const Int offset : offsets) {
    const Int indexLb = offset;
    const Int indexUb = numValues - 1 + offset;
    EXPECT_TRUE(indexLb <= indexUb);

    std::vector<Int> indexValues(numValues, 0);
    std::iota(indexValues.begin(), indexValues.end(), offset);
    std::shuffle(indexValues.begin(), indexValues.end(), rng);

    engine->open();
    const VarId index =
        engine->makeIntVar(indexValues.back(), indexLb, indexUb);
    for (size_t i = 0; i < inputs.size(); ++i) {
      inputs.at(i) = engine->makeIntVar(inputDist(gen), inputLb, inputUb);
    }
    const VarId outputId = engine->makeIntVar(inputLb, inputLb, inputUb);
    ElementVar& invariant =
        engine->makeInvariant<ElementVar>(outputId, index, inputs, offset);
    engine->close();

    for (size_t i = 0; i < indexValues.size(); ++i) {
      const Int indexVal = indexValues.at(i);
      Timestamp ts = t0 + Timestamp(i);
      EXPECT_EQ(invariant.nextInput(ts, *engine), index);
      engine->setValue(ts, index, indexVal);
      invariant.notifyCurrentInputChanged(ts, *engine);
      EXPECT_EQ(engine->value(ts, outputId), computeOutput(ts, index, offset));

      const VarId curInput = invariant.nextInput(ts, *engine);
      EXPECT_EQ(curInput, getInput(indexVal, offset));

      const Int oldInputVal = engine->value(ts, curInput);
      do {
        engine->setValue(ts, curInput, inputDist(gen));
      } while (engine->value(ts, curInput) == oldInputVal);

      invariant.notifyCurrentInputChanged(ts, *engine);
      EXPECT_EQ(engine->value(ts, outputId), computeOutput(ts, index, offset));
    }
  }
}

TEST_F(ElementVarTest, Commit) {
  EXPECT_TRUE(inputLb <= inputUb);
  for (const Int offset : offsets) {
    const Int indexLb = offset;
    const Int indexUb = numValues - 1 + offset;
    EXPECT_TRUE(indexLb <= indexUb);

    std::vector<Int> indexValues(numValues);
    std::iota(indexValues.begin(), indexValues.end(), offset);
    std::shuffle(indexValues.begin(), indexValues.end(), rng);

    engine->open();
    const VarId index =
        engine->makeIntVar(indexValues.back(), indexLb, indexUb);
    for (size_t i = 0; i < inputs.size(); ++i) {
      inputs.at(i) = engine->makeIntVar(inputDist(gen), inputLb, inputUb);
    }
    const VarId outputId = engine->makeIntVar(inputLb, inputLb, inputUb);
    ElementVar& invariant =
        engine->makeInvariant<ElementVar>(outputId, index, inputs, offset);
    engine->close();

    Int committedIndexValue = engine->committedValue(index);

    std::vector<Int> committedInputValues(inputs.size());
    for (size_t i = 0; i < committedInputValues.size(); ++i) {
      committedInputValues.at(i) = engine->committedValue(inputs.at(i));
    }

    for (size_t i = 0; i < indexValues.size(); ++i) {
      const Int indexVal = indexValues.at(i);
      Timestamp ts = engine->currentTimestamp() + Timestamp(i);
      ASSERT_EQ(engine->committedValue(index), committedIndexValue);
      for (size_t j = 0; j < inputs.size(); ++j) {
        ASSERT_EQ(engine->committedValue(inputs.at(j)),
                  committedInputValues.at(j));
      }

      // Change Index
      engine->setValue(ts, index, indexVal);

      // notify index change
      invariant.notifyInputChanged(ts, *engine, LocalId(0));

      // incremental value from index
      Int notifiedOutput = engine->value(ts, outputId);
      invariant.recompute(ts, *engine);

      ASSERT_EQ(notifiedOutput, engine->value(ts, outputId));

      // Change input
      const VarId curInput = getInput(indexVal, offset);
      const Int oldInputVal = engine->value(ts, curInput);
      do {
        engine->setValue(ts, curInput, inputDist(gen));
      } while (engine->value(ts, curInput) == oldInputVal);

      // notify input change
      invariant.notifyInputChanged(ts, *engine, LocalId(indexVal));

      // incremental value from input
      notifiedOutput = engine->value(ts, outputId);
      invariant.recompute(ts, *engine);

      ASSERT_EQ(notifiedOutput, engine->value(ts, outputId));

      engine->commitIf(ts, index);
      committedIndexValue = engine->value(ts, index);
      engine->commitIf(ts, curInput);
      committedInputValues.at(zeroBasedIndex(indexVal, offset)) =
          engine->value(ts, curInput);
      engine->commitIf(ts, outputId);

      invariant.commit(ts, *engine);
      invariant.recompute(ts + 1, *engine);
      ASSERT_EQ(notifiedOutput, engine->value(ts + 1, outputId));
    }
  }
}

class MockElementVar : public ElementVar {
 public:
  bool registered = false;
  void registerVars(Engine& engine) override {
    registered = true;
    ElementVar::registerVars(engine);
  }
  explicit MockElementVar(VarId output, VarId index,
                          std::vector<VarId> varArray, Int offset)
      : ElementVar(output, index, varArray, offset) {
    ON_CALL(*this, recompute)
        .WillByDefault([this](Timestamp timestamp, Engine& engine) {
          return ElementVar::recompute(timestamp, engine);
        });
    ON_CALL(*this, nextInput)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          return ElementVar::nextInput(t, engine);
        });
    ON_CALL(*this, notifyCurrentInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine) {
          ElementVar::notifyCurrentInputChanged(t, engine);
        });
    ON_CALL(*this, notifyInputChanged)
        .WillByDefault([this](Timestamp t, Engine& engine, LocalId id) {
          ElementVar::notifyInputChanged(t, engine, id);
        });
    ON_CALL(*this, commit).WillByDefault([this](Timestamp t, Engine& engine) {
      ElementVar::commit(t, engine);
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
TEST_F(ElementVarTest, EngineIntegration) {
  for (const auto& [propMode, markingMode] : propMarkModes) {
    if (!engine->isOpen()) {
      engine->open();
    }
    std::vector<VarId> args;
    const Int numArgs = 10;
    args.reserve(numArgs);
    for (Int value = 0; value < numArgs; ++value) {
      args.push_back(engine->makeIntVar(value, -100, 100));
    }
    VarId idx = engine->makeIntVar(0, 0, numArgs - 1);
    VarId output = engine->makeIntVar(-10, -100, 100);
    testNotifications<MockElementVar>(
        &engine->makeInvariant<MockElementVar>(output, idx, args, 1), propMode,
        markingMode, 3, idx, 5, output);
  }
}

}  // namespace
