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

#include "verilog/analysis/checkers/enum_name_style_rule.h"

#include <initializer_list>

#include "gtest/gtest.h"
#include "common/analysis/linter_test_utils.h"
#include "common/analysis/syntax_tree_linter_test_utils.h"
#include "common/text/symbol.h"
#include "verilog/CST/verilog_nonterminals.h"
#include "verilog/analysis/verilog_analyzer.h"

namespace verilog {
namespace analysis {
namespace {

using verible::LintTestCase;
using verible::RunLintTestCases;

TEST(EnumNameStyleRuleTest, ValidEnumNames) {
  const std::initializer_list<LintTestCase> kTestCases = {
      {""},
      {"typedef enum baz_t;"},
      {"typedef enum good_name_t;"},
      {"typedef enum b_a_z_t;"},
      {"typedef enum baz_e;"},
      {"typedef enum good_name_e;"},
      {"typedef enum b_a_z_e;"},
      {"typedef enum { OneValue, TwoValue } my_name_e;\nmy_name_e a_instance;"},
      {"typedef enum logic [1:0] { Fir, Oak, Pine } tree_e;\ntree_e a_tree;"},
      {"typedef enum { Red=3, Green=5 } state_e;\nstate_e a_state;"},
      {"typedef // We declare a type here"
       "enum { Idle, Busy } status_e;\nstatus_e a_status;"},
      {"typedef enum { OneValue, TwoValue } my_name_t;\nmy_name_t a_instance;"},
      {"typedef enum logic [1:0] { Fir, Oak, Pine } tree_t;\ntree_t a_tree;"},
      {"typedef enum { Red=3, Green=5 } state_t;\nstate_t a_state;"},
      {"typedef // We declare a type here"
       "enum { Idle, Busy } status_t;\nstatus_t a_status;"},
  };
  RunLintTestCases<VerilogAnalyzer, EnumNameStyleRule>(kTestCases);
}

TEST(EnumNameStyleRuleTest, InvalidEnumNames) {
  constexpr int kToken = SymbolIdentifier;
  const std::initializer_list<LintTestCase> kTestCases = {
      {"typedef enum ", {kToken, "HelloWorld"}, ";"},
      {"typedef enum ", {kToken, "_baz"}, ";"},
      {"typedef enum ", {kToken, "Bad_name"}, ";"},
      {"typedef enum ", {kToken, "bad_Name"}, ";"},
      {"typedef enum ", {kToken, "Bad2"}, ";"},
      {"typedef enum ", {kToken, "very_Bad_name"}, ";"},
      {"typedef enum ", {kToken, "wrong_ending"}, ";"},
      {"typedef enum ", {kToken, "almost_righ_T"}, ";"},
      {"typedef enum ", {kToken, "a_nam_E"}, ";"},
      {"typedef enum ", {kToken, "_t"}, ";"},
      {"typedef enum ", {kToken, "t"}, ";"},
      {"typedef enum ", {kToken, "_e"}, ";"},
      {"typedef enum ", {kToken, "e"}, ";"},
      {"typedef enum ", {kToken, "_"}, ";"},
      {"typedef enum ", {kToken, "foo_"}, ";"},
      {"typedef enum {foo, bar} ", {kToken, "HelloWorld"}, ";"},
      {"typedef enum {foo, bar} ", {kToken, "_baz"}, ";"},
      {"typedef enum {foo, bar} ", {kToken, "Bad_name"}, ";"},
      {"typedef enum {foo, bar} ", {kToken, "bad_Name"}, ";"},
      {"typedef enum {foo, bar} ", {kToken, "Bad2"}, ";"},
      {"typedef enum {foo, bar} ",
       {kToken, "very_Bad_name"},
       ";"},
      {"typedef enum {foo, bar} ",
       {kToken, "wrong_ending"},
       ";"},
      {"typedef enum {foo, bar} ", {kToken, "_t"}, ";"},
      {"typedef enum {foo, bar} ", {kToken, "t"}, ";"},
      {"typedef enum {foo, bar} ", {kToken, "_e"}, ";"},
      {"typedef enum {foo, bar} ", {kToken, "e"}, ";"},
      {"typedef enum {foo, bar} ", {kToken, "_T"}, ";"},
      {"typedef enum {foo, bar} ", {kToken, "T"}, ";"},
      {"typedef enum {foo, bar} ", {kToken, "_E"}, ";"},
      {"typedef enum {foo, bar} ", {kToken, "E"}, ";"},
      {"typedef enum {foo, bar} ", {kToken, "_"}, ";"},
      {"typedef enum {foo, bar} ", {kToken, "foo_"}, ";"},
  };
  RunLintTestCases<VerilogAnalyzer, EnumNameStyleRule>(kTestCases);
}

}  // namespace
}  // namespace analysis
}  // namespace verilog
