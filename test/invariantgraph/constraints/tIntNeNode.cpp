#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/intNeNode.hpp"

class IntNeNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(a, 5, 10);
  INT_VARIABLE(b, 2, 7);

  fznparser::Constraint constraint{"int_ne", {"a", "b"}, {}};

  fznparser::FZNModel model{{}, {a, b}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::IntNeNode> node;

  explicit IntNeNodeTest() : NodeTestBase(model) {}

  void SetUp() override {
    node = makeNode<invariantgraph::IntNeNode>(constraint);
  }
};

TEST_F(IntNeNodeTest, construction) {
  EXPECT_EQ(*node->a()->variable(),
            invariantgraph::VariableNode::FZNVariable(a));
  EXPECT_EQ(*node->b()->variable(),
            invariantgraph::VariableNode::FZNVariable(b));
  expectMarkedAsInput(node.get(), {node->a(), node->b()});
}

TEST_F(IntNeNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name, b.name});
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_FALSE(_variableMap.contains(definedVariable));
  }
  EXPECT_FALSE(_variableMap.contains(node->violation()));
  node->createDefinedVariables(engine, _variableMap);
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_TRUE(_variableMap.contains(definedVariable));
  }
  EXPECT_TRUE(_variableMap.contains(node->violation()));
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // a and b
  EXPECT_EQ(engine.searchVariables().size(), 2);

  // a, b and the violation
  EXPECT_EQ(engine.numVariables(), 3);

  // notEqual
  EXPECT_EQ(engine.numInvariants(), 1);

  EXPECT_EQ(engine.lowerBound(_variableMap.at(node->violation())), 0);
  EXPECT_EQ(engine.upperBound(_variableMap.at(node->violation())), 1);
}

static bool isViolating(std::vector<Int>& values) {
  for (size_t i = 0; i < values.size(); ++i) {
    for (size_t j = i + 1; j < values.size(); ++j) {
      if (values.at(i) == values.at(j)) {
        return true;
      }
    }
  }
  return false;
}

TEST_F(IntNeNodeTest, propagation) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name, b.name});
  node->createDefinedVariables(engine, _variableMap);
  node->registerWithEngine(engine, _variableMap);

  std::vector<VarId> inputs;
  for (auto* const inputVariable : node->inputs()) {
    EXPECT_TRUE(_variableMap.contains(inputVariable));
    inputs.emplace_back(_variableMap.at(inputVariable));
  }

  EXPECT_TRUE(_variableMap.contains(node->violation()));
  const VarId violationId = _variableMap.at(node->violation());
  EXPECT_EQ(inputs.size(), 2);

  std::vector<Int> values(inputs.size());
  engine.close();

  for (values.at(0) = engine.lowerBound(inputs.at(0));
       values.at(0) <= engine.upperBound(inputs.at(0)); ++values.at(0)) {
    for (values.at(1) = engine.lowerBound(inputs.at(1));
         values.at(1) <= engine.upperBound(inputs.at(1)); ++values.at(1)) {
      engine.beginMove();
      for (size_t i = 0; i < inputs.size(); ++i) {
        engine.setValue(inputs.at(i), values.at(i));
      }
      engine.endMove();

      engine.beginProbe();
      engine.query(violationId);
      engine.endProbe();

      EXPECT_EQ(engine.currentValue(violationId) > 0, isViolating(values));
    }
  }
}