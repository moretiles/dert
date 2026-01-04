namespace {
int tree_T_stringify(char *dest, Tree_node *src, size_t cap) {
    if(dest == NULL || src == NULL || cap == 0) {
        return EINVAL;
    }

    int res = snprintf(dest, cap, "%lu:%lu", (size_t) src, (size_t) src->val);

    return res > 0 ? 0 : res;
}

void tree_puts_debug(Tree_tree *tree, const char *string, const char *filename) {
    printf("== %s ==\n", string);
    puts("= Pre-Order =");
    ASSERT_EQ(0, tree_puts_pre(tree, tree_T_stringify, filename));
    puts("= In-Order =");
    ASSERT_EQ(0, tree_puts_in(tree, tree_T_stringify, NULL));
    puts("= Post-Order =");
    ASSERT_EQ(0, tree_puts_post(tree, tree_T_stringify, NULL));
    puts("= Breadth First Search =");
    ASSERT_EQ(0, tree_puts_bfs(tree, tree_T_stringify, NULL));
    puts("");
    puts("");
    puts("");
}

TEST(tree_T, small) {
    Tree_tree *tree;
    Tree_tree *tree_backup;

#define TREE_T_TEST_MAX_NODE (99)
    tree = tree_create(TREE_T_TEST_MAX_NODE);
    tree_backup = (Tree_tree *) malloc(tree_advise(TREE_T_TEST_MAX_NODE));
    ASSERT_NE(nullptr, tree_backup);

    // perfectly balanced tree with height of 2
    // basic insertion
    {
        // height = 0
        ASSERT_EQ(0, tree_insert(tree, 50));

        // height = 1
        ASSERT_EQ(0, tree_insert(tree, 30));
        ASSERT_EQ(0, tree_insert(tree, 70));

        // height = 2
        ASSERT_EQ(0, tree_insert(tree, 20));
        ASSERT_EQ(0, tree_insert(tree, 40));
        ASSERT_EQ(0, tree_insert(tree, 60));
        ASSERT_EQ(0, tree_insert(tree, 80));
    }

    // lookup node
    {
        Tree_node *root_node, *min_node, *max_node;
        root_node = tree_lookup(tree, 50);
        ASSERT_NE(nullptr, root_node);
        ASSERT_EQ(50, root_node->val);
        min_node = tree_lookup(tree, 20);
        ASSERT_NE(nullptr, min_node);
        ASSERT_EQ(20, min_node->val);
        max_node = tree_lookup(tree, 80);
        ASSERT_NE(nullptr, max_node);
        ASSERT_EQ(80, max_node->val);

        Tree_node *first_range_node, *smallest_range_node, *largest_range_node;
        first_range_node = tree_lookup_range(tree, 20, 30, TREE_LOOKUP_FIRST);
        ASSERT_NE(nullptr, first_range_node);
        ASSERT_EQ(30, first_range_node->val);
        smallest_range_node = tree_lookup_range(tree, 20, 30, TREE_LOOKUP_SMALLEST);
        ASSERT_NE(nullptr, smallest_range_node);
        ASSERT_EQ(20, smallest_range_node->val);
        largest_range_node = tree_lookup_range(tree, 20, 30, TREE_LOOKUP_LARGEST);
        ASSERT_NE(nullptr, largest_range_node);
        ASSERT_EQ(30, largest_range_node->val);

        min_node = tree_node_min(tree, tree->root);
        ASSERT_NE(nullptr, min_node);
        ASSERT_EQ(20, min_node->val);
        max_node = tree_node_max(tree, tree->root);
        ASSERT_NE(nullptr, max_node);
        ASSERT_EQ(80, max_node->val);
    }

    // iteration
    {
        MT_DEBUG("iterate") {
            tree_puts_debug(tree, "Perfectly balanced tree", "tests/vtree/graphs/simple/0001_initial.png");
        }
    }

    // delete
    {
        ASSERT_EQ(tree_backup, memcpy(tree_backup, tree, tree_advise(TREE_T_TEST_MAX_NODE)));
        ASSERT_EQ(0, tree_delete(tree, 50));
        ASSERT_EQ(0, tree_delete(tree, 30));
        MT_DEBUG("first_deletion") {
            tree_puts_debug(tree, "First set of deletes", "tests/vtree/graphs/simple/0002_firstdelete.png");
        }


        ASSERT_EQ(tree, memcpy(tree, tree_backup, tree_advise(TREE_T_TEST_MAX_NODE)));
        ASSERT_EQ(0, tree_delete(tree, 50));
        ASSERT_EQ(0, tree_delete(tree, 70));
        ASSERT_EQ(0, tree_delete(tree, 80));
        MT_DEBUG("second_deletion") {
            tree_puts_debug(tree, "Second set of deletes", "tests/vtree/graphs/simple/0003_secondelete.png");
        }


        ASSERT_EQ(tree, memcpy(tree, tree_backup, tree_advise(TREE_T_TEST_MAX_NODE)));
        ASSERT_EQ(0, tree_delete(tree, 50));
        ASSERT_EQ(0, tree_delete(tree, 30));
        ASSERT_EQ(0, tree_delete(tree, 20));
        ASSERT_EQ(0, tree_delete(tree, 70));
        ASSERT_EQ(0, tree_delete(tree, 80));
        ASSERT_EQ(0, tree_delete(tree, 40));
        ASSERT_EQ(0, tree_delete(tree, 60));
        MT_DEBUG("third_deletion") {
            tree_puts_debug(tree, "Only remaining node is 60", "tests/vtree/graphs/simple/0003_secondelete.png");
        }
        ASSERT_EQ(tree, memcpy(tree, tree_backup, tree_advise(TREE_T_TEST_MAX_NODE)));
    }

    // update
    {
        ASSERT_EQ(tree_backup, memcpy(tree_backup, tree, tree_advise(TREE_T_TEST_MAX_NODE)));
        ASSERT_EQ(0, tree_update(tree, 90, 50));
        MT_DEBUG("first_update") {
            tree_puts_bfs(tree, tree_T_stringify, "tests/vtree/graphs/simple/0004_afterfirstupdate.png");
        }
        ASSERT_EQ(0, tree_update(tree, 45, 70));
        MT_DEBUG("second_update") {
            tree_puts_bfs(tree, tree_T_stringify, "tests/vtree/graphs/simple/0005_aftersecondupdate.png");
        }
        ASSERT_EQ(0, tree_update(tree, 5,  40));
        MT_DEBUG("third_update") {
            tree_puts_bfs(tree, tree_T_stringify, "tests/vtree/graphs/simple/0006_afterthirdupdate.png");
        }
        ASSERT_EQ(tree, memcpy(tree, tree_backup, tree_advise(TREE_T_TEST_MAX_NODE)));
    }

    // many
    {
        uint8_t insert_vals[4] = { 15, 30, 45, 60 };
        uint8_t delete_vals[4] = { 15, 30, 45, 60 };
        uint8_t update_vals_new[4] = { 30, 40, 50, 60 };
        uint8_t update_vals_old[4] = { 20, 30, 60, 80 };
        ASSERT_EQ(tree_backup, memcpy(tree_backup, tree, tree_advise(TREE_T_TEST_MAX_NODE)));
        MT_DEBUG("before_many") {
            tree_puts_bfs(tree, tree_T_stringify, "tests/vtree/graphs/simple/0100_before_many.png");
        }
        ASSERT_EQ(0, tree_insert_many(tree, insert_vals, 4, sizeof(uint8_t), true));
        MT_DEBUG("first_many") {
            tree_puts_bfs(tree, tree_T_stringify, "tests/vtree/graphs/simple/0101_after_insert_many.png");
        }
        ASSERT_EQ(0, tree_delete_many(tree, delete_vals, 4));
        MT_DEBUG("second_many") {
            tree_puts_bfs(tree, tree_T_stringify, "tests/vtree/graphs/simple/0102_after_delete_many.png");
        }
        ASSERT_EQ(0, tree_update_many(tree, update_vals_new, update_vals_old, 4, false));
        MT_DEBUG("third_many") {
            tree_puts_bfs(tree, tree_T_stringify, "tests/vtree/graphs/simple/0103_after_update_many.png");
        }
        ASSERT_EQ(tree, memcpy(tree, tree_backup, tree_advise(TREE_T_TEST_MAX_NODE)));
    }

    tree_destroy(tree);
    free(tree_backup);
}

TEST(tree_T, big) {
    Tree_tree *tree;
    Tree_tree *tree_backup;

#define TREE_T_TEST_VALUES_LEN (60)
    const uint8_t values[TREE_T_TEST_VALUES_LEN] = { 42, 80, 42, 43, 10, 82, 95, 67, 37, 74, 88, 50, 14, 73, 98, 79, 7, 59, 37, 28, 5, 75, 94, 56, 4, 31, 1, 31, 91, 5, 45, 71, 82, 55, 72, 87, 69, 82, 60, 2, 61, 88, 66, 50, 25, 25, 42, 27, 6, 26, 92, 54, 10, 79, 61, 66, 92, 8, 79, 17 };
    tree = tree_create(TREE_T_TEST_MAX_NODE);
    tree_backup = (Tree_tree *) malloc(tree_advise(TREE_T_TEST_MAX_NODE));
    ASSERT_NE(nullptr, tree_backup);

    // insertion
    for(size_t i = 0; i < TREE_T_TEST_VALUES_LEN; i++) {
        ASSERT_EQ(0, tree_insert(tree, values[i]));
    }

    // lookup node
    {
        Tree_node *root_node, *min_node, *max_node;
        root_node = tree_lookup(tree, 42);
        ASSERT_NE(nullptr, root_node);
        ASSERT_EQ(42, root_node->val);
        min_node = tree_lookup(tree, 1);
        ASSERT_NE(nullptr, min_node);
        ASSERT_EQ(1, min_node->val);
        max_node = tree_lookup(tree, 98);
        ASSERT_NE(nullptr, max_node);
        ASSERT_EQ(98, max_node->val);

        min_node = tree_node_min(tree, tree->root);
        ASSERT_NE(nullptr, min_node);
        ASSERT_EQ(1, min_node->val);
        max_node = tree_node_max(tree, tree->root);
        ASSERT_NE(nullptr, max_node);
        ASSERT_EQ(98, max_node->val);

        ASSERT_TRUE(tree_reachable(tree));
    }

    // iteration
    {
        // bfs is working
        MT_DEBUG("bfs") {
            tree_puts_bfs(tree, tree_T_stringify, "tests/vtree/graphs/bigtree/0001_initial.png");
        }

        // preorder gets into infinite loop
        //tree_puts_pre(tree, tree_T_stringify, NULL);

        // inorder misses 98 (the largest value on the tree)
        //tree_puts_post(tree, tree_T_stringify, NULL);

        // not sure about postorder
        //tree_puts_in(tree, tree_T_stringify, NULL);
    }

    // delete
    {
        //assert(memcpy(tree_backup, tree, tree_advise(TREE_T_TEST_MAX_NODE)) == tree_backup);
        //assert(tree_delete(tree, 50) == 0);
        //assert(tree_delete(tree, 30) == 0);
        //tree_puts_debug(tree, "First set of deletes", "tests/vtree/graphs/simple/0002_firstdelete.png");


        //assert(memcpy(tree, tree_backup, tree_advise(TREE_T_TEST_MAX_NODE)) == tree);
        //assert(tree_delete(tree, 56) == 0);
        //assert(tree_delete(tree, 31) == 0);
        //assert(tree_delete(tree, 82) == 0);
        //assert(tree_min(tree) != NULL);
        //assert(tree_max(tree) != NULL);
        //assert(tree_reachable(tree) == true);
        //tree_puts_debug(tree, "Second set of deletes", "tests/vtree/graphs/simple/0003_secondelete.png");


        //assert(memcpy(tree, tree_backup, tree_advise(TREE_T_TEST_MAX_NODE)) == tree);
        //assert(tree_delete(tree, 50) == 0);
        //assert(tree_delete(tree, 30) == 0);
        //assert(tree_delete(tree, 20) == 0);
        //assert(tree_delete(tree, 70) == 0);
        //assert(tree_delete(tree, 80) == 0);
        //assert(tree_delete(tree, 40) == 0);
        //assert(tree_delete(tree, 60) == 0);
        //tree_puts_debug(tree, "Only remaining node is 60", "tests/vtree/graphs/simple/0003_secondelete.png");
        //assert(memcpy(tree, tree_backup, tree_advise(TREE_T_TEST_MAX_NODE)) == tree);
    }

    // update
    //{
    //assert(tree_update(tree, 90, 50) == 0);
    //assert(tree_update(tree, 45, 70) == 0);
    //assert(tree_update(tree, 5,  40) == 0);
    //}

    tree_destroy(tree);
    free(tree_backup);
}
}
