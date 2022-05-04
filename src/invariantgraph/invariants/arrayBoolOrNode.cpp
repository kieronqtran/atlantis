#include "invariantgraph/invariants/arrayBoolOrNode.hpp"

#include "../parseHelper.hpp"
#include "invariants/exists.hpp"
#include "views/violation2BoolView.hpp"

std::unique_ptr<invariantgraph::ArrayBoolOrNode>
invariantgraph::ArrayBoolOrNode::fromModelConstraint(
    const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(constraint.name == "array_bool_or");
  assert(constraint.arguments.size() == 2);

  auto as = mappedVariableVector(model, constraint.arguments[0], variableMap);
  auto r = mappedVariable(constraint.arguments[1], variableMap);

  return std::make_unique<invariantgraph::ArrayBoolOrNode>(as, r);
}

void invariantgraph::ArrayBoolOrNode::createDefinedVariables(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  if (_violationVarId == NULL_ID) {
    _violationVarId = engine.makeIntVar(0, 0, 0);
    assert(!variableMap.contains(definedVariables()[0]));
    variableMap.emplace(
        definedVariables()[0],
        engine.makeIntView<Violation2BoolView>(_violationVarId));
  }
}

void invariantgraph::ArrayBoolOrNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  std::vector<VarId> inputs;
  std::transform(_as.begin(), _as.end(), std::back_inserter(inputs),
                 [&](const auto& node) { return variableMap.at(node); });
#ifndef NDEBUG
  for (const VarId input : inputs) {
    assert(0 <= engine.lowerBound(input));
  }
#endif
  engine.makeInvariant<Exists>(inputs, _violationVarId);
}
