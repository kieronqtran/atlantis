#include "invariantgraph/constraints/boolXorNode.hpp"

#include "../parseHelper.hpp"
#include "constraints/notEqual.hpp"

std::unique_ptr<invariantgraph::BoolXorNode>
invariantgraph::BoolXorNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(
      (constraint.name == "bool_xor" && constraint.arguments.size() == 2) ||
      (constraint.name == "bool_xor_reif" && constraint.arguments.size() == 3));

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);
  VariableNode* r = constraint.arguments.size() >= 3
                        ? mappedVariable(constraint.arguments[2], variableMap)
                        : nullptr;

  return std::make_unique<BoolXorNode>(a, b, r);
}

void invariantgraph::BoolXorNode::createDefinedVariables(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  if (!variableMap.contains(violation())) {
    registerViolation(engine, variableMap);
  }
}

void invariantgraph::BoolXorNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  assert(variableMap.contains(violation()));

  engine.makeConstraint<::NotEqual>(variableMap.at(violation()),
                                    variableMap.at(a()), variableMap.at(b()));
}