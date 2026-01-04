#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct tree_node *tree_iterator_pre_find_smallest_above(Tree_node *current);
struct tree_node *tree_iterator_pre_next(struct tree_iterator *iterator);

#ifdef __cplusplus
}
#endif
