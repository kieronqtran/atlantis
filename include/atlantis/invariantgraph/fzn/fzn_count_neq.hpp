#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"

namespace atlantis::invariantgraph::fzn {

bool fzn_count_neq(FznInvariantGraph&,
                   const std::shared_ptr<fznparser::IntVarArray>& inputs,
                   const fznparser::IntArg& needle,
                   const fznparser::IntArg& count);

bool fzn_count_neq(FznInvariantGraph&,
                   const std::shared_ptr<fznparser::IntVarArray>& inputs,
                   const fznparser::IntArg& needle,
                   const fznparser::IntArg& count,
                   const fznparser::BoolArg& reified);

bool fzn_count_neq(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
