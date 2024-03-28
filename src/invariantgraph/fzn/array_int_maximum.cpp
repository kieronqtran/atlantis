#include "atlantis/invariantgraph/fzn/array_int_maximum.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayIntMaximumNode.hpp"
#include "atlantis/invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_int_maximum(FznInvariantGraph& invariantGraph,
                       const fznparser::IntArg& output,
                       const std::shared_ptr<fznparser::IntVarArray>& inputs) {
  invariantGraph.addInvariantNode(std::make_unique<ArrayIntMaximumNode>(
      invariantGraph.retrieveVarNodes(inputs),
      invariantGraph.retrieveVarNode(output)));
  return true;
}

bool array_int_maximum(FznInvariantGraph& invariantGraph,
                       const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "array_int_maximum") {
    return false;
  }
  verifyNumArguments(constraint, 2);
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 1, fznparser::IntVarArray, true)

  return array_int_maximum(
      invariantGraph, std::get<fznparser::IntArg>(constraint.arguments().at(0)),
      std::get<std::shared_ptr<fznparser::IntVarArray>>(
          constraint.arguments().at(1)));
}

}  // namespace atlantis::invariantgraph::fzn
