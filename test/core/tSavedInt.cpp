#include <iostream>
#include <limits>
#include <random>
#include <vector>
#include <stdexcept>

#include "core/savedInt.hpp"
#include "core/types.hpp"
#include "gtest/gtest.h"

namespace {
class SavedIntTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    std::random_device rd;
    gen = std::mt19937(rd());
  }
  std::mt19937 gen;
};

/**
 *  Testing constructor
 */

TEST_F(SavedIntTest, SavedIntConstructor) {
  std::uniform_int_distribution<> distribution(std::numeric_limits<int>::min(), std::numeric_limits<int>::max() - 1);

  // Random timestamp
  Timestamp initTime = std::max(0, distribution(gen));
  // Random inital value
  Int value = distribution(gen);

  SavedInt savedInt = SavedInt(initTime, value);

  // Get the current value at the initial timestamp return the initial value
  EXPECT_EQ(savedInt.getValue(initTime), value);

  // get at the current value at the initial timestamp. Returns the initial
  // value
  EXPECT_EQ(savedInt.getValue(initTime), value);

  Timestamp otherTime = std::max(0, distribution(gen));
  // Get the current value at another timestamp.
  // Should still return the initial value (as no other value has been
  // committed)
  EXPECT_EQ(savedInt.getValue(otherTime), value);

  // get the current value at another timestamp. Should still return the
  // initial value (as no other value has been committed)
  EXPECT_EQ(savedInt.getValue(otherTime), value);

  EXPECT_THROW(
    SavedInt(initTime, value, 10, 0),
    std::out_of_range
  );

}

TEST_F(SavedIntTest, SavedIntSetGetValue) {
  // A uniform distribution for the initial value and timestamp
  std::uniform_int_distribution<> distribution1(std::numeric_limits<int>::min(), 10000);
  // A uniform distribution for the other (next) timestamp
  std::uniform_int_distribution<> distribution2(10001, std::numeric_limits<int>::max());

  Timestamp initTime = std::max(0, distribution1(gen));
  Int initValue = distribution1(gen);

  Timestamp nextTime = distribution2(gen);
  Int nextValue = distribution2(gen);

  // Create a SavedInt
  SavedInt savedInt = SavedInt(initTime, initValue);

  // Update the value for the other (next) timestamp
  savedInt.setValue(nextTime, nextValue);

  // The value at the initial timestamp is still the initial value
  EXPECT_EQ(savedInt.getValue(initTime), initValue);
  // The value at the next timestamp is now the next value
  EXPECT_EQ(savedInt.getValue(nextTime), nextValue);

  Timestamp otherTime = distribution2(gen);
  while (otherTime == nextTime) {
    otherTime = distribution2(gen);
  }
  // The value for any timestamp except nextTime is still the initial value
  EXPECT_EQ(savedInt.getValue(otherTime), initValue);
}

TEST_F(SavedIntTest, SavedIntIncValue) {
  std::uniform_int_distribution<> distribution(std::numeric_limits<int>::min() + 10, std::numeric_limits<int>::max() - 10);

  Timestamp initTime = std::max(0, distribution(gen));
  Int committedValue = distribution(gen);

  SavedInt savedInt = SavedInt(initTime, committedValue);

  Timestamp nextTime;
  Int nextValue;
  // Increase -10, -9, ..., -1, 1, 2, ..., 10
  for (Int increment = -10; increment <= 10; ++increment) {
    if (increment == 0) {
      continue;
    }
    nextTime = initTime + increment;
    nextValue = committedValue + increment;

    // Not the same timestamp as init timestamp, increment will be based on:
    //   saved
    savedInt.incValue(nextTime, increment);

    EXPECT_EQ(savedInt.getValue(initTime), committedValue);
    EXPECT_EQ(savedInt.getValue(nextTime), nextValue);

    // Not the same timestamp as init timestamp, increment will be based on:
    //   tmp
    // NOTE: tmp was already changed above
    savedInt.incValue(nextTime, increment);

    Timestamp otherTime = std::max(0, distribution(gen));
    while (otherTime == nextTime) {
      otherTime = distribution(gen);
    }
    // Any time except nextTime returns the committed value.
    EXPECT_EQ(savedInt.getValue(otherTime), committedValue);
    // nextTime returns the twice incremented value
    EXPECT_EQ(savedInt.getValue(nextTime), nextValue + increment);
  }
}

TEST_F(SavedIntTest, SavedIntCommitValue) {
  std::uniform_int_distribution<> distribution1(std::numeric_limits<int>::min(), 10000);
  std::uniform_int_distribution<> distribution2(10001, std::numeric_limits<int>::max());

  Timestamp initTime = std::max(0, distribution1(gen));
  Int initValue = distribution1(gen);

  Timestamp nextTime = distribution2(gen);
  Int commitedValue = distribution2(gen);

  SavedInt savedInt = SavedInt(initTime, initValue);

  savedInt.commitValue(commitedValue);

  EXPECT_EQ(savedInt.getValue(initTime), initValue);
  EXPECT_EQ(savedInt.getValue(nextTime), commitedValue);
}

TEST_F(SavedIntTest, SavedIntCommit) {
  std::uniform_int_distribution<> distribution1(std::numeric_limits<int>::min(), 10000);
  std::uniform_int_distribution<> distribution2(10001, std::numeric_limits<int>::max());

  Timestamp initTime = std::max(0, distribution1(gen));
  Int initValue = distribution1(gen);

  Timestamp nextTime = distribution2(gen);
  Int commitedValue = distribution2(gen);

  SavedInt savedInt = SavedInt(initTime, initValue);

  EXPECT_EQ(savedInt.getValue(initTime), initValue);
  EXPECT_EQ(savedInt.getValue(nextTime), initValue);

  savedInt.setValue(nextTime, commitedValue);

  EXPECT_EQ(savedInt.getValue(initTime), initValue);
  EXPECT_EQ(savedInt.getValue(nextTime), commitedValue);

  savedInt.commitIf(nextTime);

  EXPECT_EQ(savedInt.getValue(initTime), commitedValue);
  EXPECT_EQ(savedInt.getValue(nextTime), commitedValue);
}

TEST_F(SavedIntTest, SavedIntCommitIf) {
  std::uniform_int_distribution<> distribution1(std::numeric_limits<int>::min(), 10000);
  std::uniform_int_distribution<> distribution2(10001, std::numeric_limits<int>::max());

  Timestamp initTime = std::max(0, distribution1(gen));
  Int initValue = distribution1(gen);

  Timestamp nextTime = distribution2(gen);
  Int nextValue = distribution2(gen);

  SavedInt savedInt = SavedInt(initTime, initValue);

  savedInt.commitIf(nextTime);

  EXPECT_EQ(savedInt.getValue(initTime), initValue);
  EXPECT_EQ(savedInt.getValue(initTime), initValue);
  EXPECT_EQ(savedInt.getValue(nextTime), initValue);
  EXPECT_EQ(savedInt.getValue(nextTime), initValue);

  savedInt.commitIf(initTime);

  EXPECT_EQ(savedInt.getValue(initTime), initValue);
  EXPECT_EQ(savedInt.getValue(initTime), initValue);
  EXPECT_EQ(savedInt.getValue(nextTime), initValue);
  EXPECT_EQ(savedInt.getValue(nextTime), initValue);

  savedInt.setValue(nextTime, nextValue);

  savedInt.commitIf(nextTime);

  EXPECT_EQ(savedInt.getValue(initTime), nextValue);
  EXPECT_EQ(savedInt.getValue(initTime), nextValue);
  EXPECT_EQ(savedInt.getValue(nextTime), nextValue);
  EXPECT_EQ(savedInt.getValue(nextTime), nextValue);

  savedInt.commitIf(initTime);

  EXPECT_EQ(savedInt.getValue(initTime), nextValue);
  EXPECT_EQ(savedInt.getValue(initTime), nextValue);
  EXPECT_EQ(savedInt.getValue(nextTime), nextValue);
  EXPECT_EQ(savedInt.getValue(nextTime), nextValue);
}

TEST_F(SavedIntTest, InDomain) {
    
  Int lowerBound = -10;
  Int upperBound = 10;
  Timestamp timestamp = Timestamp(1);
  SavedInt savedInt = SavedInt(timestamp, 0, lowerBound, upperBound);

  for (Int value = lowerBound; value <= upperBound; ++value) {
    EXPECT_TRUE(savedInt.inDomain(value));
  }

  for (Int value = lowerBound - 1000; value < lowerBound; ++value) {
    EXPECT_FALSE(savedInt.inDomain(value));
  }

  for (Int value = upperBound + 1; value < upperBound + 1000; ++value) {
    EXPECT_FALSE(savedInt.inDomain(value));
  }
}

TEST_F(SavedIntTest, UpdateDomain) {  
  Int initialLowerBound = 0;
  Int initialUpperBound = 0;
  
  Timestamp timestamp = Timestamp(1);
  SavedInt savedInt = SavedInt(timestamp, 0, initialLowerBound, initialUpperBound);

  for (Int value = 1; value <= 1000; ++value) {
    EXPECT_FALSE(savedInt.inDomain(-value));
    EXPECT_FALSE(savedInt.inDomain(value));
    savedInt.updateDomain(-value, value);
    EXPECT_TRUE(savedInt.inDomain(-value));
    EXPECT_TRUE(savedInt.inDomain(value));
  }

  EXPECT_THROW(
    savedInt.updateDomain(10, -10),
    std::out_of_range
  );
}


}  // namespace