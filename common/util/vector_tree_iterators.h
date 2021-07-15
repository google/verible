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

#ifndef VERIBLE_COMMON_UTIL_VECTOR_TREE_ITERATORS_H_
#define VERIBLE_COMMON_UTIL_VECTOR_TREE_ITERATORS_H_

#include <iterator>

#include "common/util/iterator_range.h"
#include "common/util/logging.h"
#include "common/util/vector_tree.h"

namespace verible {

// Class implementing common VectorTree*Iterator members using CRTP polymorphic
// chaining. Derived class must implement following methods:
// - `static VectorTreeType* _NextNode(VectorTreeType* node)` - returns pointer
//   to the next node
template <typename ImplType, typename VectorTreeType>
class _VectorTreeIteratorBase {
 public:
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = VectorTreeType;
  using pointer = VectorTreeType*;
  using reference = VectorTreeType&;

  _VectorTreeIteratorBase(pointer node) : node_(node) {}

  reference operator*() const {
    CHECK_NOTNULL(node_);
    return *node_;
  }
  pointer operator->() const {
    CHECK_NOTNULL(node_);
    return node_;
  }
  ImplType& operator++() {
    node_ = ImplType::_NextNode(node_);
    return static_cast<ImplType&>(*this);
  }
  ImplType operator++(int) {
    ImplType tmp = static_cast<ImplType&>(*this);
    node_ = ImplType::_NextNode(node_);
    return tmp;
  }
  friend ImplType operator+(ImplType lhs, std::size_t rhs) {
    while (rhs--) ++lhs;
    return lhs;
  }
  friend bool operator==(const ImplType& a, const ImplType& b) {
    return a.node_ == b.node_;
  };
  friend bool operator!=(const ImplType& a, const ImplType& b) {
    return a.node_ != b.node_;
  };

 protected:
  pointer node_;
};

template <typename VectorTreeType>
class VectorTreeLeavesIterator
    : public _VectorTreeIteratorBase<VectorTreeLeavesIterator<VectorTreeType>,
                                     VectorTreeType> {
  using this_type = VectorTreeLeavesIterator<VectorTreeType>;
  using base_type = _VectorTreeIteratorBase<this_type, VectorTreeType>;

 public:
  VectorTreeLeavesIterator(VectorTreeType* node)
      : base_type(node ? node->LeftmostDescendant() : nullptr) {}

  static VectorTreeType* _NextNode(VectorTreeType* node) {
    if (!node) return nullptr;
    return node->NextLeaf();
  }
};

template <typename VectorTreeType>
iterator_range<VectorTreeLeavesIterator<VectorTreeType>>
VectorTreeLeavesTraversal(VectorTreeType& tree) {
  VectorTreeLeavesIterator<VectorTreeType> begin(tree.LeftmostDescendant());
  VectorTreeLeavesIterator<VectorTreeType> end(tree.RightmostDescendant());
  ++end;
  return {begin, end};
}

template <typename VectorTreeType>
class VectorTreePreOrderIterator
    : public _VectorTreeIteratorBase<VectorTreePreOrderIterator<VectorTreeType>,
                                     VectorTreeType> {
  using this_type = VectorTreePreOrderIterator<VectorTreeType>;
  using base_type = _VectorTreeIteratorBase<this_type, VectorTreeType>;

 public:
  VectorTreePreOrderIterator(VectorTreeType* node) : base_type(node) {}

  static VectorTreeType* _NextNode(VectorTreeType* node) {
    if (!node) return nullptr;
    if (!node->is_leaf()) return &node->Children().front();
    while (node && node->IsLastChild()) {
      node = node->Parent();
    }
    if (node) node = node->NextSibling();
    return node;
  }

  this_type begin() const { return *this; }
  this_type end() const {
    if (!this->node_) return this_type(nullptr);
    return this_type(_NextNode(this->node_->RightmostDescendant()));
  }
};

template <typename VectorTreeType>
iterator_range<VectorTreePreOrderIterator<VectorTreeType>>
VectorTreePreOrderTraversal(VectorTreeType& tree) {
  VectorTreePreOrderIterator<VectorTreeType> it(&tree);
  return {it.begin(), it.end()};
}

template <typename VectorTreeType>
class VectorTreePostOrderIterator
    : public _VectorTreeIteratorBase<
          VectorTreePostOrderIterator<VectorTreeType>, VectorTreeType> {
  using this_type = VectorTreePostOrderIterator<VectorTreeType>;
  using base_type = _VectorTreeIteratorBase<this_type, VectorTreeType>;

 public:
  VectorTreePostOrderIterator(VectorTreeType* node) : base_type(node) {}

  static VectorTreeType* _NextNode(VectorTreeType* node) {
    if (!node) return nullptr;
    if (node->IsLastChild()) return node->Parent();
    node = node->NextSibling();
    if (!node->is_leaf()) node = node->LeftmostDescendant();
    return node;
  }

  this_type begin() const {
    if (!this->node_) return this_type(nullptr);
    return this_type(this->node_->LeftmostDescendant());
  }
  this_type end() const { return this_type(_NextNode(this->node_)); }
};

template <typename VectorTreeType>
iterator_range<VectorTreePostOrderIterator<VectorTreeType>>
VectorTreePostOrderTraversal(VectorTreeType& tree) {
  VectorTreePostOrderIterator<VectorTreeType> it(&tree);
  return {it.begin(), it.end()};
}

}  // namespace verible

#endif  // VERIBLE_COMMON_UTIL_VECTOR_TREE_ITERATORS_H_
