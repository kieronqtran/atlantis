#include <gmock/gmock.h>

#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/constraints/allDifferentNode.hpp"

class AllDifferentNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(a, 5, 10);
  INT_VARIABLE(b, 2, 7);
  INT_VARIABLE(c, 2, 7);
  INT_VARIABLE(d, 2, 7);

  fznparser::Constraint constraint{
      "alldifferent",
      {fznparser::Constraint::ArrayArgument{"a", "b", "c", "d"}},
      {}};
  fznparser::FZNModel model{
      {}, {a, b, c, d}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::AllDifferentNode> node;

  AllDifferentNodeTest() : NodeTestBase(model) {}

  void SetUp() override {
    node = makeNode<invariantgraph::AllDifferentNode>(constraint);
  }
};

TEST_F(AllDifferentNodeTest, construction) {
  std::vector<invariantgraph::VariableNode*> expectedVars;
  std::transform(_variables.begin(), _variables.end(),
                 std::back_inserter(expectedVars),
                 [](const auto& variable) { return variable.get(); });

  EXPECT_EQ(node->variables(), expectedVars);
  EXPECT_THAT(node->inputs(), testing::ContainerEq(node->variables()));

  expectMarkedAsInput(node.get(), node->inputs());
}

TEST_F(AllDifferentNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name, b.name, c.name, d.name});
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_FALSE(_variableMap.contains(definedVariable));
  }
  EXPECT_FALSE(_variableMap.contains(node->violation()));
  node->createDefinedVariables(engine, _variableMap);
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_TRUE(_variableMap.contains(definedVariable));
  }
  EXPECT_TRUE(_variableMap.contains(node->violation()));
  EXPECT_TRUE(_variableMap.contains(node->violation()));
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // a, b, c and d
  EXPECT_EQ(engine.searchVariables().size(), 4);

  // a, b, c, d and the violation
  EXPECT_EQ(engine.numVariables(), 5);

  // alldifferent
  EXPECT_EQ(engine.numInvariants(), 1);

  EXPECT_EQ(engine.lowerBound(_variableMap.at(node->violation())), 0);
  EXPECT_EQ(engine.upperBound(_variableMap.at(node->violation())), 3);
}

static bool isViolating(const std::vector<Int>& values) {
  for (size_t i = 0; i < values.size(); i++) {
    for (size_t j = i + 1; j < values.size(); j++) {
      if (values.at(i) == values.at(j)) {
        return true;
      }
    }
  }
  return false;
}

TEST_F(AllDifferentNodeTest, propagation) {
  PropagationEngine engine;
  engine.open();
  registerVariables(engine, {a.name, b.name, c.name, d.name});
  node->createDefinedVariables(engine, _variableMap);
  node->registerWithEngine(engine, _variableMap);

  std::vector<VarId> inputs;
  for (auto* const inputVariable : node->inputs()) {
    EXPECT_TRUE(_variableMap.contains(inputVariable));
    inputs.emplace_back(_variableMap.at(inputVariable));
  }

  EXPECT_TRUE(_variableMap.contains(node->violation()));
  const VarId violationId = _variableMap.at(node->violation());
  EXPECT_EQ(inputs.size(), 4);

  std::vector<Int> values(inputs.size());
  engine.close();

  for (values.at(0) = engine.lowerBound(inputs.at(0));
       values.at(0) <= engine.upperBound(inputs.at(0)); ++values.at(0)) {
    for (values.at(1) = engine.lowerBound(inputs.at(1));
         values.at(1) <= engine.upperBound(inputs.at(1)); ++values.at(1)) {
      for (values.at(2) = engine.lowerBound(inputs.at(2));
           values.at(2) <= engine.upperBound(inputs.at(2)); ++values.at(2)) {
        for (values.at(3) = engine.lowerBound(inputs.at(3));
             values.at(3) <= engine.upperBound(inputs.at(3)); ++values.at(3)) {
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
  }
}