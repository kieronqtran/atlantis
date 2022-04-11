#pragma once

#include <fznparser/model.hpp>

#include "../structure.hpp"

namespace invariantgraph {

class ArrayVarIntElementNode : public VariableDefiningNode {
 private:
  std::vector<VariableNode*> _as;
  VariableNode* _b;

 public:
  static std::unique_ptr<ArrayVarIntElementNode> fromModelConstraint(
      const fznparser::FZNModel& model, const fznparser::Constraint& constraint,
      const std::function<VariableNode*(MappableValue&)>& variableMap);

  ArrayVarIntElementNode(std::vector<VariableNode*> as, VariableNode* b,
                         VariableNode* output)
      : VariableDefiningNode({output}, {as}), _as(std::move(as)), _b(b) {
    // No way to add this as an input in addition to the as vector. So we do it
    // here explicitly.
    b->markAsInputFor(static_cast<VariableDefiningNode*>(this));

    Int outputLb = std::numeric_limits<Int>::max();
    Int outputUb = std::numeric_limits<Int>::min();

    for (const auto& node : _as) {
      const auto& [nodeLb, nodeUb] = node->bounds();
      outputLb = std::min(nodeLb, outputLb);
      outputUb = std::max(nodeUb, outputUb);
    }

    b->imposeDomain(IntervalDomain{1, static_cast<Int>(_as.size())});
    output->imposeDomain(IntervalDomain{outputLb, outputUb});
  }

  void registerWithEngine(
      Engine& engine, VariableDefiningNode::VariableMap& variableMap) override;

  [[nodiscard]] const std::vector<VariableNode*>& as() const noexcept {
    return _as;
  }
  [[nodiscard]] VariableNode* b() const noexcept { return _b; }
};

}  // namespace invariantgraph