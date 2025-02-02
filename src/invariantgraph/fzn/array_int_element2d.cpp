#include "atlantis/invariantgraph/fzn/array_int_element2d.hpp"

#include "../parseHelper.hpp"
#include "./fznHelper.hpp"
#include "atlantis/invariantgraph/invariantNodes/arrayElement2dNode.hpp"
#include "atlantis/invariantgraph/types.hpp"

namespace atlantis::invariantgraph::fzn {

bool array_int_element2d(FznInvariantGraph& graph,
                         const fznparser::IntArg& idx1,
                         const fznparser::IntArg& idx2,
                         std::vector<Int>&& parVector,
                         const fznparser::IntArg& output, const Int numRows,
                         const Int offset1, const Int offset2) {
  if (numRows <= 0 || parVector.size() % numRows != 0) {
    throw FznArgumentException(
        "Constraint array_int_element2d the number of rows must be strictly "
        "positive and a divide the number of elements in the array.");
  }

  if (offset1 >
      (idx1.isParameter() ? idx1.toParameter() : idx1.var()->lowerBound())) {
    throw FznArgumentException(
        "Constraint array_int_element2d the first offset must be smaller than "
        "the lower bound of the first index var.");
  }

  if (offset2 >
      (idx2.isParameter() ? idx2.toParameter() : idx2.var()->lowerBound())) {
    throw FznArgumentException(
        "Constraint array_int_element2d the second offset must be smaller than "
        "the lower bound of the second index var.");
  }

  const size_t numCols = parVector.size() / static_cast<size_t>(numRows);

  std::vector<std::vector<Int>> parMatrix(numRows, std::vector<Int>(numCols));
  for (Int i = 0; i < numRows; ++i) {
    for (size_t j = 0; j < numCols; ++j) {
      parMatrix.at(i).at(j) = parVector.at(i * numCols + j);
    }
  }

  graph.addInvariantNode(std::make_shared<ArrayElement2dNode>(
      graph, graph.retrieveVarNode(idx1), graph.retrieveVarNode(idx2),
      std::move(parMatrix), graph.retrieveVarNode(output), offset1, offset2));
  return true;
}

bool array_int_element2d(FznInvariantGraph& graph,
                         const fznparser::Constraint& constraint) {
  if (constraint.identifier() != "array_int_element2d" &&
      constraint.identifier() != "array_int_element2d_nonshifted_flat") {
    return false;
  }
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 0, fznparser::IntArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 1, fznparser::IntArg, true)
  FZN_CONSTRAINT_ARRAY_TYPE_CHECK(constraint, 2, fznparser::IntVarArray, false)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 3, fznparser::IntArg, true)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 4, fznparser::IntArg, false)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 5, fznparser::IntArg, false)
  FZN_CONSTRAINT_TYPE_CHECK(constraint, 6, fznparser::IntArg, false)

  return array_int_element2d(
      graph, std::get<fznparser::IntArg>(constraint.arguments().at(0)),
      std::get<fznparser::IntArg>(constraint.arguments().at(1)),
      getArgArray<fznparser::IntVarArray>(constraint.arguments().at(2))
          ->toParVector(),
      std::get<fznparser::IntArg>(constraint.arguments().at(3)),
      std::get<fznparser::IntArg>(constraint.arguments().at(4)).toParameter(),
      std::get<fznparser::IntArg>(constraint.arguments().at(5)).toParameter(),
      std::get<fznparser::IntArg>(constraint.arguments().at(6)).toParameter());
}

}  // namespace atlantis::invariantgraph::fzn
