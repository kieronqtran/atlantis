#include <iostream>
#include <random>
#include <vector>
#include "gtest/gtest.h"
#include "domain/intDomain.hpp"
#include <bits/stdc++.h> 
#include <iostream>

// The fixture for testing class IntDomain. From google test primer.
class IntDomainTest : public ::testing::Test {
protected:
    // You can remove any or all of the following functions if its body
    // is empty.

    IntDomainTest() {
        std::random_device rd;
        gen = std::mt19937(rd());
        // You can do set-up work for each test here.
    }

    virtual ~IntDomainTest() {
        // You can do clean-up work that doesn't throw exceptions here.
    }

    // If the constructor and destructor are not enough for setting up
    // and cleaning up each test, you can define the following methods:
    virtual void SetUp() {
        // Code here will be called immediately after the constructor (right
        // before each test).
    }

    virtual void TearDown() {
        // Code here will be called immediately after each test (right
        // before the destructor).
    }

    std::mt19937 gen;
    // Objects declared here can be used by all tests in the test case for IntDomain.
};

TEST_F(IntDomainTest, SingleIntDomain) {
    std::uniform_int_distribution<> distribution(INT_MIN + 1, INT_MAX - 1);
    
    int value = distribution(gen);
    IntDomain intDomain = IntDomain(value, value);
    EXPECT_EQ(1, intDomain.domainSize);
    
    EXPECT_TRUE(intDomain.containsValue(value));
    
    EXPECT_FALSE(intDomain.containsValue(value - 1));
    std::uniform_int_distribution<> distributionLesser(INT_MIN, value - 1);
    for (size_t i = 0; i < 10; i++) {
        EXPECT_FALSE(intDomain.containsValue(distributionLesser(gen)));
    }

    EXPECT_FALSE(intDomain.containsValue(value + 1));
    std::uniform_int_distribution<> distributionGreater(value + 1, INT_MAX);
    for (size_t i = 0; i < 10; i++) {
        EXPECT_FALSE(intDomain.containsValue(distributionGreater(gen)));
    }
}

TEST_F(IntDomainTest, IntDomain) {
    int rangeSize = 10;
    std::uniform_int_distribution<> distribution(INT_MIN + 1, INT_MAX - 1 - rangeSize);
    
    int minValue = distribution(gen);
    int maxValue = minValue + rangeSize - 1;

    IntDomain intDomain = IntDomain(minValue, maxValue);
    
    EXPECT_EQ(rangeSize, intDomain.domainSize);

    for (int value = minValue; value <= maxValue; value++) {    
        EXPECT_TRUE(intDomain.containsValue(value));
    }

    std::uniform_int_distribution<> distributionLesser(INT_MIN, minValue - 1);
    for (size_t i = 0; i < 10; i++) {
        EXPECT_FALSE(intDomain.containsValue(distributionLesser(gen)));
    }

    std::uniform_int_distribution<> distributionGreater(maxValue + 1, INT_MAX);
    for (size_t i = 0; i < 10; i++) {
        EXPECT_FALSE(intDomain.containsValue(distributionGreater(gen)));
    }

}

TEST_F(IntDomainTest, IntDomainMerge) {
    int rangeSize = 10;
    std::uniform_int_distribution<> distribution(INT_MIN + 1, INT_MAX - 1 - rangeSize);
    
    int minValue1 = distribution(gen);
    int maxValue1 = minValue1 + rangeSize - 1;

    int minValue2 = distribution(gen);
    int maxValue2 = minValue2 + rangeSize - 1;

    IntDomain intDomain1 = IntDomain(minValue1, maxValue1);
    EXPECT_EQ(rangeSize, intDomain1.domainSize);
    
    IntDomain intDomain2 = IntDomain(minValue2, maxValue2);
    EXPECT_EQ(rangeSize, intDomain2.domainSize);

    intDomain1.merge(intDomain2);
    EXPECT_EQ(intDomain1.lowerBound, std::min(minValue1, minValue2));
    EXPECT_EQ(intDomain1.upperBound, std::max(maxValue1, maxValue2));
    EXPECT_EQ(std::max(maxValue1, maxValue2) - std::min(minValue1, minValue2) + 1, intDomain1.domainSize);
}