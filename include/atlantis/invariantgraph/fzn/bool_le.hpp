#pragma once

#include <fznparser/constraint.hpp>
#include <fznparser/variables.hpp>

#include "atlantis/invariantgraph/fznInvariantGraph.hpp"

namespace atlantis::invariantgraph::fzn {

bool bool_le(FznInvariantGraph&, const fznparser::BoolArg& a,
             const fznparser::BoolArg& b);

bool bool_le(FznInvariantGraph&, const fznparser::BoolArg& a,
             const fznparser::BoolArg& b, const fznparser::BoolArg& reified);

bool bool_le(FznInvariantGraph&, const fznparser::Constraint&);

}  // namespace atlantis::invariantgraph::fzn
