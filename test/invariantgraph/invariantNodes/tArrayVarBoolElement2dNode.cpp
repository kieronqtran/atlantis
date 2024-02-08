#include "../nodeTestBase.hpp"
#include "invariantgraph/fzn/array_var_bool_element2d.hpp"
#include "invariantgraph/invariantNodes/arrayVarElement2dNode.hpp"
#include "propagation/solver.hpp"

namespace atlantis::testing {

using namespace atlantis::invariantgraph;

class ArrayVarBoolElement2dNodeTest
    : public NodeTestBase<ArrayVarElement2dNode> {
 public:
  VarNodeId x00{NULL_NODE_ID};
  VarNodeId x01{NULL_NODE_ID};
  VarNodeId x10{NULL_NODE_ID};
  VarNodeId x11{NULL_NODE_ID};

  VarNodeId idx1{NULL_NODE_ID};
  VarNodeId idx2{NULL_NODE_ID};
  VarNodeId output{NULL_NODE_ID};

  Int offsetIdx1 = 1;
  Int offsetIdx2 = 1;

  void SetUp() override {
    NodeTestBase::SetUp();
    x00 = createBoolVarNode("x00");
    x01 = createBoolVarNode("x01");
    x10 = createBoolVarNode("x10");
    x11 = createBoolVarNode("x11");
    idx1 = createIntVarNode(1, 2, "idx1");
    idx2 = createIntVarNode(1, 2, "idx2");
    output = createBoolVarNode("output", true);

    std::vector<std::vector<VarNodeId>> argMatrix{{x00, x01}, {x10, x11}};

    createInvariantNode(idx1, idx2, std::move(argMatrix), output, offsetIdx1,
                        offsetIdx2);
  }
};

TEST_F(ArrayVarBoolElement2dNodeTest, construction) {
  expectInputTo(invNode());
  expectOutputOf(invNode());

  EXPECT_EQ(invNode().idx1(), idx1);
  EXPECT_EQ(invNode().idx2(), idx2);

  EXPECT_EQ(invNode().outputVarNodeIds().size(), 1);
  EXPECT_EQ(invNode().outputVarNodeIds().front(), output);

  EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), 4);
  EXPECT_EQ(invNode().dynamicInputVarNodeIds().at(0), x00);
  EXPECT_EQ(invNode().dynamicInputVarNodeIds().at(1), x01);
  EXPECT_EQ(invNode().dynamicInputVarNodeIds().at(2), x10);
  EXPECT_EQ(invNode().dynamicInputVarNodeIds().at(3), x11);
}

TEST_F(ArrayVarBoolElement2dNodeTest, application) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_EQ(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerOutputVars(*_invariantGraph, solver);
  for (const auto& outputVarNodeId : invNode().outputVarNodeIds()) {
    EXPECT_NE(varId(outputVarNodeId), propagation::NULL_ID);
  }
  invNode().registerNode(*_invariantGraph, solver);
  solver.close();

  // x00, x01, x10, x11, idx1, idx2
  EXPECT_EQ(solver.searchVars().size(), 6);

  // x00, x01, x10, x11, idx1, idx2, and output
  EXPECT_EQ(solver.numVars(), 7);

  // element2dVar
  EXPECT_EQ(solver.numInvariants(), 1);
}

TEST_F(ArrayVarBoolElement2dNodeTest, propagation) {
  propagation::Solver solver;
  solver.open();
  addInputVarsToSolver(solver);
  invNode().registerOutputVars(*_invariantGraph, solver);
  invNode().registerNode(*_invariantGraph, solver);

  EXPECT_EQ(invNode().staticInputVarNodeIds().size(), 2);
  EXPECT_NE(varId(invNode().staticInputVarNodeIds().front()),
            propagation::NULL_ID);

  EXPECT_EQ(invNode().dynamicInputVarNodeIds().size(), 4);
  for (const auto& inputVarNodeId : invNode().dynamicInputVarNodeIds()) {
    EXPECT_NE(varId(inputVarNodeId), propagation::NULL_ID);
  }

  EXPECT_NE(varId(invNode().outputVarNodeIds().front()), propagation::NULL_ID);
  const propagation::VarId outputId =
      varId(invNode().outputVarNodeIds().front());

  std::vector<propagation::VarId> inputVars;
  inputVars.emplace_back(varId(invNode().idx1()));
  inputVars.emplace_back(varId(invNode().idx2()));
  for (const auto& varNodeId : invNode().dynamicInputVarNodeIds()) {
    inputVars.emplace_back(varId(varNodeId));
    solver.updateBounds(varId(varNodeId), 0, 3, true);
  }
  solver.close();
  std::vector<Int> inputVals = makeInputVals(solver, inputVars);

  while (increaseNextVal(solver, inputVars, inputVals)) {
    solver.beginMove();
    setVarVals(solver, inputVars, inputVals);
    solver.endMove();

    solver.beginProbe();
    solver.query(outputId);
    solver.endProbe();

    const Int actual = solver.currentValue(outputId);
    const Int row = inputVals.at(0) - offsetIdx1;  // offset of 1
    const Int col = inputVals.at(1) - offsetIdx2;  // offset of 1

    const Int index = 2 + (row * 2 + col);

    EXPECT_EQ(actual, inputVals.at(index));
  }
}

}  // namespace atlantis::testing