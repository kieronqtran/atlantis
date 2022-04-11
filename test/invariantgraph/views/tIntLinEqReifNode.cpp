#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/views/intLinEqReifNode.hpp"

class IntLinEqReifNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(a, 5, 10);
  INT_VARIABLE(b, 2, 7);
  INT_VARIABLE(r, 0, 1);
  Int c{3};

  fznparser::Constraint constraint{
      "int_lin_eq_reif",
      {fznparser::Constraint::ArrayArgument{1, -1},
       fznparser::Constraint::ArrayArgument{"a", "b"}, c, "r"},
      {}};

  fznparser::FZNModel model{{}, {a, b, r}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::IntLinEqReifNode> node;

  IntLinEqReifNodeTest() : NodeTestBase(model) {}

  void SetUp() override {
    node = makeNode<invariantgraph::IntLinEqReifNode>(constraint);
  }
};

TEST_F(IntLinEqReifNodeTest, construction) {
  EXPECT_EQ(node->definedVariables().size(), 1);
  EXPECT_EQ(*node->definedVariables()[0]->variable(),
            invariantgraph::VariableNode::FZNVariable(r));
}

TEST_F(IntLinEqReifNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name, b.name});
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // a, b
  EXPECT_EQ(engine.searchVariables().size(), 3);

  // a, b and r
  EXPECT_EQ(engine.numVariables(), 5);
}
