#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <deque>
#include <random>
#include <vector>

#include "atlantis/invariantgraph/fzn/int_mod.hpp"

namespace atlantis::testing {

using ::testing::AtLeast;
using ::testing::AtMost;

using namespace atlantis::invariantgraph;
using namespace atlantis::invariantgraph::fzn;

class int_modTest : public ::testing::Test {
 public:
  std::vector<VarNodeId> inputVarNodeIds{};
  Int numInputs = 3;

  void SetUp() override {}
};

}  // namespace atlantis::testing
