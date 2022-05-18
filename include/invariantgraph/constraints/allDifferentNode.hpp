#pragma once

#include <fznparser/model.hpp>
#include <utility>

#include "invariantgraph/softConstraintNode.hpp"

namespace invariantgraph {
class AllDifferentNode : public SoftConstraintNode {
 private:
  VarId _intermediate{NULL_ID};

 public:
  explicit AllDifferentNode(std::vector<VariableNode*> variables,
                            VariableNode* r)
      : SoftConstraintNode(variables, r) {}

  explicit AllDifferentNode(std::vector<VariableNode*> variables,
                            bool shouldHold)
      : SoftConstraintNode(variables, shouldHold) {}

  static std::unique_ptr<AllDifferentNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  void createDefinedVariables(Engine& engine) override;

  void registerWithEngine(Engine& engine) override;
};
}  // namespace invariantgraph