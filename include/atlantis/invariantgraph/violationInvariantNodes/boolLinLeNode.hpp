#pragma once

#include "atlantis/invariantgraph/violationInvariantNode.hpp"

namespace atlantis::invariantgraph {

class BoolLinLeNode : public ViolationInvariantNode {
 private:
  std::vector<Int> _coeffs;
  Int _bound;
  propagation::VarViewId _intermediate{propagation::NULL_ID};

 public:
  BoolLinLeNode(IInvariantGraph& graph, std::vector<Int>&& coeffs,
                std::vector<VarNodeId>&& vars, Int bound,
                bool shouldHold = true);

  BoolLinLeNode(IInvariantGraph& graph, std::vector<Int>&& coeffs,
                std::vector<VarNodeId>&& vars, Int bound, VarNodeId reified);

  void init(InvariantNodeId) override;

  void updateState() override;

  void registerOutputVars() override;

  void registerNode() override;

  [[nodiscard]] const std::vector<Int>& coeffs() const;
};

}  // namespace atlantis::invariantgraph
