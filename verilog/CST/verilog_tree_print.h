// Copyright 2017-2020 The Verible Authors.
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

// VerilogPrettyPrinter is a specialized printer for Verilog syntax trees.

#ifndef VERIBLE_VERILOG_CST_VERILOG_TREE_PRINT_H_
#define VERIBLE_VERILOG_CST_VERILOG_TREE_PRINT_H_

#include <iosfwd>

#include "common/text/concrete_syntax_leaf.h"
#include "common/text/concrete_syntax_tree.h"
#include "common/text/symbol.h"
#include "common/text/tree_utils.h"

#include "common/text/visitors.h"
#include "json/json.h"

namespace verilog {

class VerilogPrettyPrinter : public verible::PrettyPrinter {
 public:
  explicit VerilogPrettyPrinter(std::ostream* output_stream,
                                absl::string_view base);

  void Visit(const verible::SyntaxTreeLeaf&) override;
  void Visit(const verible::SyntaxTreeNode&) override;

  // void Visit(verible::SyntaxTreeNode*) override;
};

// Prints tree contained at root to stream
void PrettyPrintVerilogTree(const verible::Symbol& root, absl::string_view base,
                            std::ostream* stream);

class VerilogTreeToJsonConverter : public verible::SymbolVisitor {
 public:
  explicit VerilogTreeToJsonConverter(absl::string_view base);

  void Visit(const verible::SyntaxTreeLeaf&) override;
  void Visit(const verible::SyntaxTreeNode&) override;

  Json::Value get_json() { return json_; }

 protected:
  // Range of text spanned by syntax tree, used for offset calculation.
  const verible::TokenInfo::Context context_;

  // JSON tree root
  Json::Value json_;

  // Pointer to JSON value of currently visited symbol in its parent's
  // children list.
  Json::Value* value_;
};

Json::Value ConvertVerilogTreeToJson(const verible::Symbol& root, absl::string_view base);

}  // namespace verilog

#endif  // VERIBLE_VERILOG_CST_VERILOG_TREE_PRINT_H_
