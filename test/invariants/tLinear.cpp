#include <iostream>
#include <limits>
#include <random>
#include <vector>

#include "core/propagationEngine.hpp"
#include "core/savedInt.hpp"
#include "core/types.hpp"
#include "gtest/gtest.h"
#include "invariants/linear.hpp"

namespace {
class LinearTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    std::random_device rd;
    gen = std::mt19937(rd());
    e = std::make_unique<PropagationEngine>();
    e->open();
    a = e->makeIntVar(1, -100, 100);
    b = e->makeIntVar(2, -100, 100);
    c = e->makeIntVar(3, -100, 100);
    d = e->makeIntVar(4, -100, 100);

    // d = 1*1+2*10+3*(-20) = 1+20-60 =-39
    linear = e->makeInvariant<Linear>(std::vector<Int>({1, 10, -20}),
                                      std::vector<VarId>({a, b, c}), d);
    e->close();
  }
  std::unique_ptr<PropagationEngine> e;
  VarId a = NULL_ID;
  VarId b = NULL_ID;
  VarId c = NULL_ID;
  VarId d = NULL_ID;
  std::shared_ptr<Linear> linear;
  std::mt19937 gen;
};

/**
 *  Testing constructor
 */

TEST_F(LinearTest, Init) {
  EXPECT_EQ(e->getCommittedValue(d), -39);
  EXPECT_EQ(e->getValue(e->getTmpTimestamp(d), d), -39);
}

TEST_F(LinearTest, Recompute) {
  EXPECT_EQ(e->getValue(0, d), -39);
  EXPECT_EQ(e->getCommittedValue(d), -39);

  Timestamp newTime = 1;
  e->updateValue(newTime, a, 40);
  linear->recompute(newTime, *e);
  EXPECT_EQ(e->getCommittedValue(d), -39);
  EXPECT_EQ(e->getValue(newTime, d), 0);
}

TEST_F(LinearTest, NotifyChange) {
  EXPECT_EQ(e->getValue(0, d), -39);  // initially the value of d is -39

  Timestamp time1 = 1;

  EXPECT_EQ(e->getValue(time1, a), 1);
  e->updateValue(time1, a, 40);
  EXPECT_EQ(e->getCommittedValue(a), 1);
  EXPECT_EQ(e->getValue(time1, a), 40);
  linear->notifyIntChanged(time1, *e, 0, e->getValue(time1, a));
  EXPECT_EQ(e->getValue(time1, d), 0);  // incremental value of d is 0;

  e->updateValue(time1, b, 0);
  linear->notifyIntChanged(time1, *e, 1, e->getValue(time1, b));
  auto tmpValue = e->getValue(time1, d);  // incremental value of d is -40;

  // Incremental computation gives the same result as recomputation
  linear->recompute(time1, *e);
  EXPECT_EQ(e->getValue(time1, d), tmpValue);

  Timestamp time2 = time1 + 1;

  EXPECT_EQ(e->getValue(time2, a), 1);
  e->updateValue(time2, a, 20);
  EXPECT_EQ(e->getCommittedValue(a), 1);
  EXPECT_EQ(e->getValue(time2, a), 20);
  linear->notifyIntChanged(time2, *e, 0, e->getValue(time2, a));
  EXPECT_EQ(e->getValue(time2, d), -20);  // incremental value of d is 0;
}

TEST_F(LinearTest, IncrementalVsRecompute) {
  EXPECT_EQ(e->getValue(0, d), -39);  // initially the value of d is -39
  // todo: not clear if we actually want to deal with overflows...
  std::uniform_int_distribution<> distribution(-100000, 100000);

  Timestamp currentTime = 1;
  for (size_t i = 0; i < 1000; ++i) {
    ++currentTime;
    // Check that we do not accidentally commit
    ASSERT_EQ(e->getCommittedValue(a), 1);
    ASSERT_EQ(e->getCommittedValue(b), 2);
    ASSERT_EQ(e->getCommittedValue(c), 3);
    ASSERT_EQ(e->getCommittedValue(d), -39);  // d is committed by register.

    // Set all variables
    e->updateValue(currentTime, a, distribution(gen));
    e->updateValue(currentTime, b, distribution(gen));
    e->updateValue(currentTime, c, distribution(gen));

    // notify changes
    if (e->getCommittedValue(a) != e->getValue(currentTime, a)) {
      linear->notifyIntChanged(currentTime, *e, 0, e->getValue(currentTime, a));
    }
    if (e->getCommittedValue(b) != e->getValue(currentTime, b)) {
      linear->notifyIntChanged(currentTime, *e, 1, e->getValue(currentTime, b));
    }
    if (e->getCommittedValue(c) != e->getValue(currentTime, c)) {
      linear->notifyIntChanged(currentTime, *e, 2, e->getValue(currentTime, c));
    }

    // incremental value
    auto tmp = e->getValue(currentTime, d);
    linear->recompute(currentTime, *e);

    ASSERT_EQ(tmp, e->getValue(currentTime, d));
  }
}

TEST_F(LinearTest, Commit) {
  EXPECT_EQ(e->getCommittedValue(d), -39);

  Timestamp currentTime = 1;

  e->updateValue(currentTime, a, 40);
  e->updateValue(currentTime, b, 2);  // This change is not notified and should
                                      // not have an impact on the commit

  linear->notifyIntChanged(currentTime, *e, 0, e->getValue(currentTime, a));

  // Committing an invariant does not commit its output!
  // // Commit at wrong timestamp should have no impact
  // linear->commit(currentTime + 1, *e);
  // EXPECT_EQ(e->getCommittedValue(d), -39);
  // linear->commit(currentTime, *e);
  // EXPECT_EQ(e->getCommittedValue(d), 0);
}
}  // namespace
