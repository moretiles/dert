#pragma once

struct tree_node *tree_iterator_in_find_smallest_below(Tree_node *current);
struct tree_node *tree_iterator_in_find_smallest_above(Tree_node *current);
struct tree_node *tree_iterator_in_next(struct tree_iterator *iterator);
