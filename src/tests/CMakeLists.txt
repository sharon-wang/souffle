# Souffle - A Datalog Compiler
# Copyright (c) 2021 The Souffle Developers. All rights reserved
# Licensed under the Universal Permissive License v 1.0 as shown at:
# - https://opensource.org/licenses/UPL
# - <souffle root>/licenses/SOUFFLE-UPL.txt

include(SouffleTests)

souffle_add_binary_test(binary_relation_test src)
souffle_add_binary_test(brie_test src)
souffle_add_binary_test(btree_multiset_test src)
souffle_add_binary_test(btree_set_test src)
souffle_add_binary_test(compiled_tuple_test src)
souffle_add_binary_test(eqrel_datastructure_test src)
souffle_add_binary_test(graph_utils_test src)
souffle_add_binary_test(parallel_utils_test src)
souffle_add_binary_test(profile_util_test src)
souffle_add_binary_test(record_table_test src)
souffle_add_binary_test(symbol_table_test src)
souffle_add_binary_test(table_test src)
souffle_add_binary_test(util_test src)

