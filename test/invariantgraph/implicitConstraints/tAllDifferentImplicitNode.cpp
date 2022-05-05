#include "../nodeTestBase.hpp"
#include "core/propagationEngine.hpp"
#include "invariantgraph/implicitConstraints/allDifferentImplicitNode.hpp"
#include "search/neighbourhoods/allDifferentNeighbourhood.hpp"

class AllDifferentImplicitNodeTest : public NodeTestBase {
 public:
  INT_VARIABLE(a, 2, 7);
  INT_VARIABLE(b, 2, 7);
  INT_VARIABLE(c, 2, 7);
  INT_VARIABLE(d, 2, 7);

  fznparser::Constraint constraint{
      "alldifferent",
      {fznparser::Constraint::ArrayArgument{"a", "b", "c", "d"}},
      {}};

  fznparser::FZNModel model{
      {}, {a, b, c, d}, {constraint}, fznparser::Satisfy{}};

  std::unique_ptr<invariantgraph::AllDifferentImplicitNode> node;

  void SetUp() override {
    setModel(&model);
    node = makeNode<invariantgraph::AllDifferentImplicitNode>(constraint);
  }
};

TEST_F(AllDifferentImplicitNodeTest, construction) {
  std::vector<invariantgraph::VariableNode*> expectedVars;
  std::transform(_variables.begin(), _variables.end(),
                 std::back_inserter(expectedVars),
                 [](const auto& variable) { return variable.get(); });

  EXPECT_EQ(node->definedVariables(), expectedVars);
  EXPECT_FALSE(node->violation());
}

TEST_F(AllDifferentImplicitNodeTest, application) {
  PropagationEngine engine;
  engine.open();
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_FALSE(_variableMap.contains(definedVariable));
  }
  node->createDefinedVariables(engine, _variableMap);
  for (auto* const definedVariable : node->definedVariables()) {
    EXPECT_TRUE(_variableMap.contains(definedVariable));
  }
  node->registerWithEngine(engine, _variableMap);
  engine.close();

  // a, b, c and d
  EXPECT_EQ(engine.searchVariables().size(), 4);

  // a, b, c and d
  EXPECT_EQ(engine.numVariables(), 4);

  EXPECT_EQ(engine.numInvariants(), 0);

  auto neighbourhood = node->takeNeighbourhood();
  EXPECT_FALSE(node->takeNeighbourhood());

  EXPECT_TRUE(dynamic_cast<search::neighbourhoods::AllDifferentNeighbourhood*>(
      neighbourhood.get()));
}
