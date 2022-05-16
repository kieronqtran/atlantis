#include "invariantgraph/views/bool2IntNode.hpp"

#include "../parseHelper.hpp"
#include "views/bool2IntView.hpp"

std::unique_ptr<invariantgraph::Bool2IntNode>
invariantgraph::Bool2IntNode::fromModelConstraint(
    const fznparser::FZNModel&, const fznparser::Constraint& constraint,
    const std::function<VariableNode*(MappableValue&)>& variableMap) {
  auto a = mappedVariable(constraint.arguments[0], variableMap);
  auto b = mappedVariable(constraint.arguments[1], variableMap);

  return std::make_unique<invariantgraph::Bool2IntNode>(a, b);
}

void invariantgraph::Bool2IntNode::createDefinedVariables(Engine& engine) {
  if (definedVariables().front()->varId() == NULL_ID) {
    definedVariables().front()->setVarId(
        engine.makeIntView<::Bool2IntView>(input()->varId()));
  }
}

void invariantgraph::Bool2IntNode::registerWithEngine(Engine&) {}
