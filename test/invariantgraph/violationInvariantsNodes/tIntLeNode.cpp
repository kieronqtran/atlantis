#include "../nodeTestBase.hpp"
#include "invariantgraph/violationInvariantNodes/intLeNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

static bool isViolating(const std::vector<Int>& values) {
  return values.at(0) > values.at(1);
}

template <ViolationInvariantType Type>
class AbstractIntLeNodeTest : public NodeTestBase<IntLeNode> {
 public:
  VarNodeId a{NULL_NODE_ID};
  VarNodeId b{NULL_NODE_ID};
  VarNodeId reified{NULL_NODE_ID};

  void SetUp() override {
    NodeTestBase::SetUp();
    a = createIntVarNode(5, 10, "a");
    b = createIntVarNode(2, 7, "b");
    if constexpr (Type == ViolationInvariantType::REIFIED) {
      reified = createBoolVarNode("reified", true);
      createInvariantNode(a, b, reified);
    } else if constexpr (Type == ViolationInvariantType::CONSTANT_TRUE) {
      createInvariantNode(a, b, true);
    } else {
      createInvariantNode(a, b, false);
    }
  }

  void construction() {
    expectInputTo(invNode());
    expectOutputOf(invNode());

    EXPECT_EQ(invNode().a(), a);
    EXPECT_EQ(invNode().b(), b);

    if constexpr (Type != ViolationInvariantType::REIFIED) {
      EXPECT_FALSE(invNode().isReified());
      EXPECT_EQ(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
    } else {
      EXPECT_TRUE(invNode().isReified());
      EXPECT_NE(invNode().reifiedViolationNodeId(), NULL_NODE_ID);
      EXPECT_EQ(invNode().reifiedViolationNodeId(), reified);
    }
  }

  void application() {
    propagation::Solver solver;
    solver.open();
    addInputVarsToSolver(solver);
    for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
      EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
    }
    EXPECT_EQ(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    invNode().registerOutputVars(*_invariantGraph, solver);
    for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
      EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
    }
    EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    invNode().registerNode(*_invariantGraph, solver);
    solver.close();

    // a and b
    EXPECT_EQ(solver.searchVars().size(), 2);

    // a, b and the violation
    EXPECT_EQ(solver.numVars(), 3);

    // less equal
    EXPECT_EQ(solver.numInvariants(), 1);

    EXPECT_EQ(solver.lowerBound(invNode().violationVarId(*_invariantGraph)), 0);
    EXPECT_GT(solver.upperBound(invNode().violationVarId(*_invariantGraph)), 0);
  }

  void propagation() {
    propagation::Solver solver;
    solver.open();
    addInputVarsToSolver(solver);
    invNode().registerOutputVars(*_invariantGraph, solver);
    invNode().registerNode(*_invariantGraph, solver);

    std::vector<propagation::VarId> inputVars;
    EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 2);
    for (const auto& inputVarNodeId : invNode().staticInputVarNodeIds()) {
      EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
      inputVars.emplace_back(varId(inputVarNodeId));
    }

    EXPECT_NE(invNode().violationVarId(*_invariantGraph), propagation::NULL_ID);
    const propagation::VarId violationId =
        invNode().violationVarId(*_invariantGraph);
    EXPECT_EQ(inputVars.size(), 2);

    solver.close();

    std::vector<Int> inputVals = makeInputVals(solver, inputVars);

    while (increaseNextVal(solver, inputVars, inputVals)) {
      solver.beginMove();
      setVarVals(solver, inputVars, inputVals);
      solver.endMove();

      solver.beginProbe();
      solver.query(violationId);
      solver.endProbe();

      const Int actual = solver.currentValue(violationId) > 0;
      const Int expected = isViolating(inputVals);

      if constexpr (Type != ViolationInvariantType::CONSTANT_FALSE) {
        EXPECT_EQ(actual, expected);
      } else {
        EXPECT_NE(actual, expected);
      }
    }
  }
};

class IntLeNodeTest
    : public AbstractIntLeNodeTest<ViolationInvariantType::CONSTANT_TRUE> {};

TEST_F(IntLeNodeTest, Construction) { construction(); }

TEST_F(IntLeNodeTest, Application) { application(); }

TEST_F(IntLeNodeTest, Propagation) { propagation(); }

class IntLeReifNodeTest
    : public AbstractIntLeNodeTest<ViolationInvariantType::REIFIED> {};

TEST_F(IntLeReifNodeTest, Construction) { construction(); }

TEST_F(IntLeReifNodeTest, Application) { application(); }

TEST_F(IntLeReifNodeTest, Propagation) { propagation(); }

class IntLeFalseNodeTest
    : public AbstractIntLeNodeTest<ViolationInvariantType::CONSTANT_FALSE> {};

TEST_F(IntLeFalseNodeTest, Construction) { construction(); }

TEST_F(IntLeFalseNodeTest, Application) { application(); }

TEST_F(IntLeFalseNodeTest, Propagation) { propagation(); }

class IntLeTrueNodeTest
    : public AbstractIntLeNodeTest<ViolationInvariantType::CONSTANT_TRUE> {};

TEST_F(IntLeTrueNodeTest, Construction) { construction(); }

TEST_F(IntLeTrueNodeTest, Application) { application(); }

TEST_F(IntLeTrueNodeTest, Propagation) { propagation(); }

}  // namespace atlantis::testing