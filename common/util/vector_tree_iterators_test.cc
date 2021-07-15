// Copyright 2017-2021 The Verible Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "common/util/vector_tree_iterators.h"

#include <cstddef>
#include <iterator>
#include <sstream>
#include <vector>

#include "absl/strings/str_join.h"
#include "common/util/vector_tree.h"
#include "common/util/vector_tree_test_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace verible {
namespace {

using Tree = VectorTree<int>;

struct TestCaseData {
  Tree tree;

  // RootNodeTraversal test data
  struct {
    std::vector<int> expected_sequence_leaves;
    std::vector<int> expected_sequence_pre_order;
    std::vector<int> expected_sequence_post_order;
  } root_node_traversal;

  // SubtreeTraversal and IteratorSubtreeTraversal test data
  struct {
    std::vector<size_t> subtree_path;
    std::vector<int> expected_sequence_leaves;
    std::vector<int> expected_sequence_pre_order;
    std::vector<int> expected_sequence_post_order;
  } subtree_traversal;
};

static const TestCaseData kTestCasesData[] = {
    {
        Tree(0),
        // RootNodeTraversal test data
        {
            {0},
            {0},
            {0},
        },
        // SubtreeTraversal and IteratorSubtreeTraversal test data (skipped)
        {},
    },
    {
        Tree(0,        //
             Tree(1),  //
             Tree(2),  //
             Tree(3)),
        // RootNodeTraversal test data
        {
            {1, 2, 3},
            {0, 1, 2, 3},
            {1, 2, 3, 0},
        },
        // SubtreeTraversal and IteratorSubtreeTraversal test data
        {
            {0},  // subtree path
            {1},
            {1},
            {1},
        },
    },
    {
        Tree(0,                   //
             Tree(1,              //
                  Tree(11,        //
                       Tree(111,  //
                            Tree(1111))))),
        // RootNodeTraversal test data
        {
            {1111},
            {0, 1, 11, 111, 1111},
            {1111, 111, 11, 1, 0},
        },
        // SubtreeTraversal and IteratorSubtreeTraversal test data
        {
            {0, 0},  // subtree path
            {1111},
            {11, 111, 1111},
            {1111, 111, 11},
        },
    },
    {
        Tree(0,                      //
             Tree(1,                 //
                  Tree(11,           //
                       Tree(111),    //
                       Tree(112)),   //
                  Tree(12),          //
                  Tree(13)),         //
             Tree(2,                 //
                  Tree(21),          //
                  Tree(22),          //
                  Tree(23,           //
                       Tree(231),    //
                       Tree(232))),  //
             Tree(3)),
        // RootNodeTraversal test data
        {
            {111, 112, 12, 13, 21, 22, 231, 232, 3},
            {0, 1, 11, 111, 112, 12, 13, 2, 21, 22, 23, 231, 232, 3},
            {111, 112, 11, 12, 13, 1, 21, 22, 231, 232, 23, 2, 3, 0},
        },
        // SubtreeTraversal and IteratorSubtreeTraversal test data
        {
            {0},  // subtree path
            {111, 112, 12, 13},
            {1, 11, 111, 112, 12, 13},
            {111, 112, 11, 12, 13, 1},
        },
    },
    {
        Tree(0,                     //
             Tree(1),               //
             Tree(2,                //
                  Tree(21,          //
                       Tree(211),   //
                       Tree(212)),  //
                  Tree(22),         //
                  Tree(23)),        //
             Tree(3,                //
                  Tree(31),         //
                  Tree(32),         //
                  Tree(33,          //
                       Tree(331),   //
                       Tree(332)))),
        // RootNodeTraversal test data
        {
            {1, 211, 212, 22, 23, 31, 32, 331, 332},
            {0, 1, 2, 21, 211, 212, 22, 23, 3, 31, 32, 33, 331, 332},
            {1, 211, 212, 21, 22, 23, 2, 31, 32, 331, 332, 33, 3, 0},
        },
        // SubtreeTraversal and IteratorSubtreeTraversal test data
        {
            {2},  // subtree path
            {31, 32, 331, 332},
            {3, 31, 32, 33, 331, 332},
            {31, 32, 331, 332, 33, 3},
        },
    },
};

template <typename NodesRange, typename ValuesRange>
void ExpectNodesRangeValuesEq(const NodesRange& nodes,
                              const ValuesRange& expected_values) {
  auto expected_it = expected_values.begin();

  for (const auto& node : nodes) {
    EXPECT_NE(expected_it, expected_values.end());
    EXPECT_EQ(node.Value(), *expected_it);
    ++expected_it;
  }
  EXPECT_EQ(expected_it, expected_values.end());
}

TEST(VectorTreeIteratorTest, IteratorInterface) {
  // TODO: write test
}

TEST(VectorTreeIteratorTest, RootNodeTraversal) {
  for (const auto& data : kTestCasesData) {
    std::ostringstream trace_msg;
    trace_msg << "Input tree:\n" << data.tree;
    SCOPED_TRACE(trace_msg.str());

    {
      SCOPED_TRACE("VectorTreeLeavesTraversal");
      ExpectNodesRangeValuesEq(
          VectorTreeLeavesTraversal(data.tree),
          data.root_node_traversal.expected_sequence_leaves);
    }
    {
      SCOPED_TRACE("VectorTreePreOrderTraversal");
      ExpectNodesRangeValuesEq(
          VectorTreePreOrderTraversal(data.tree),
          data.root_node_traversal.expected_sequence_pre_order);
    }
    {
      SCOPED_TRACE("VectorTreePostOrderTraversal");
      ExpectNodesRangeValuesEq(
          VectorTreePostOrderTraversal(data.tree),
          data.root_node_traversal.expected_sequence_post_order);
    }
  }
}

TEST(VectorTreeIteratorTest, SubtreeTraversal) {
  for (const auto& data : kTestCasesData) {
    const auto subtree_path = data.subtree_traversal.subtree_path;
    if (subtree_path.empty()) continue;

    const auto& subtree =
        data.tree.DescendPath(subtree_path.begin(), subtree_path.end());

    std::ostringstream trace_msg;
    trace_msg << "Input tree:\n"
              << data.tree
              << "\nSubtree path: " << absl::StrJoin(subtree_path, ".");
    SCOPED_TRACE(trace_msg.str());

    {
      SCOPED_TRACE("VectorTreeLeavesTraversal");
      ExpectNodesRangeValuesEq(VectorTreeLeavesTraversal(subtree),
                               data.subtree_traversal.expected_sequence_leaves);
    }
    {
      SCOPED_TRACE("VectorTreePreOrderTraversal");
      ExpectNodesRangeValuesEq(
          VectorTreePreOrderTraversal(subtree),
          data.subtree_traversal.expected_sequence_pre_order);
    }
    {
      SCOPED_TRACE("VectorTreePostOrderTraversal");
      ExpectNodesRangeValuesEq(
          VectorTreePostOrderTraversal(subtree),
          data.subtree_traversal.expected_sequence_post_order);
    }
  }
}

TEST(VectorTreeIteratorTest, IteratorSubtreeTraversal) {
  for (const auto& data : kTestCasesData) {
    const auto subtree_path = data.subtree_traversal.subtree_path;
    if (subtree_path.empty()) continue;

    const auto& subtree =
        data.tree.DescendPath(subtree_path.begin(), subtree_path.end());

    std::ostringstream trace_msg;
    trace_msg << "Input tree:\n"
              << data.tree
              << "\nSubtree path: " << absl::StrJoin(subtree_path, ".");
    SCOPED_TRACE(trace_msg.str());

    // VectorTreeLeavesIterator doesn't support subranges
    {
      SCOPED_TRACE("VectorTreePreOrderIterator");
      ExpectNodesRangeValuesEq(
          VectorTreePreOrderIterator(&subtree),
          data.subtree_traversal.expected_sequence_pre_order);
    }
    {
      SCOPED_TRACE("VectorTreePostOrderIterator");
      ExpectNodesRangeValuesEq(
          VectorTreePostOrderIterator(&subtree),
          data.subtree_traversal.expected_sequence_post_order);
    }
  }
}

}  // namespace
}  // namespace verible
