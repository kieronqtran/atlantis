#include "invariantgraph/constraints/eqNode.hpp"

#include "../parseHelper.hpp"
#include "constraints/equal.hpp"

std::unique_ptr<invariantgraph::EqNode>
invariantgraph::EqNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(((constraint.name == "int_eq" || constraint.name == "bool_eq") &&
          constraint.arguments.size() == 2) ||
         ((constraint.name == "int_eq_reif" || constraint.name == "bool_eq") &&
          constraint.arguments.size() == 3));
  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);
  VariableNode* r = constraint.arguments.size() >= 3
                        ? mappedVariable(constraint.arguments[2], variableMap)
                        : nullptr;

  return std::make_unique<EqNode>(a, b, r);
}

void invariantgraph::EqNode::createDefinedVariables(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  if (!variableMap.contains(violation())) {
    registerViolation(engine, variableMap);
  }
}

void invariantgraph::EqNode::registerWithEngine(
    Engine& engine, VariableDefiningNode::VariableMap& variableMap) {
  assert(variableMap.contains(violation()));

  engine.makeConstraint<::Equal>(variableMap.at(violation()),
                                 variableMap.at(a()), variableMap.at(b()));
}