#pragma once

#include "atlantis/propagation/types.hpp"

namespace atlantis::propagation {

struct PropagationListNode {
  VarId id;
  size_t priority;
  PropagationListNode* next{nullptr};

  PropagationListNode() : PropagationListNode(NULL_ID, 0) {}
  PropagationListNode(VarId t_id, size_t t_priority)
      : id(t_id), priority(t_priority) {}
};

}  // namespace atlantis::propagation
