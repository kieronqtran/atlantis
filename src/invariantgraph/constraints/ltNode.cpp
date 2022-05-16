#include "invariantgraph/constraints/ltNode.hpp"

#include "../parseHelper.hpp"
#include "constraints/lessThan.hpp"

std::unique_ptr<invariantgraph::LtNode>
invariantgraph::LtNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  assert(
      (constraint.name == "int_ne" && constraint.arguments.size() == 2) ||
      (constraint.name == "int_ne_reif" && constraint.arguments.size() == 3));

  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);

  VariableNode* r = constraint.arguments.size() >= 3
                        ? mappedVariable(constraint.arguments[2], variableMap)
                        : nullptr;

  return std::make_unique<LtNode>(a, b, r);
}

void invariantgraph::LtNode::createDefinedVariables(Engine& engine) {
  registerViolation(engine);
}

void invariantgraph::LtNode::registerWithEngine(Engine& engine) {
  assert(violationVarId() != NULL_ID);

  engine.makeConstraint<::LessThan>(violationVarId(), a()->varId(),
                                    b()->varId());
}