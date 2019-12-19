// Copyright 2017-2019 The Verible Authors.
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

#ifndef VERIBLE_VERILOG_ANALYSIS_CHECKERS_ENDIF_COMMENT_RULE_H_
#define VERIBLE_VERILOG_ANALYSIS_CHECKERS_ENDIF_COMMENT_RULE_H_

#include <stack>
#include <string>
#include <set>

#include "common/analysis/lint_rule_status.h"
#include "common/analysis/token_stream_lint_rule.h"
#include "common/text/token_info.h"
#include "verilog/analysis/descriptions.h"

namespace verilog {
namespace analysis {

// Detects whether a Verilog `endif directive is followed by a comment that
// matches the opening `ifdef or `ifndef.
//
// Accepted examples:
//   `ifdef FOO
//   `endif  // FOO
//
//   `ifndef BAR
//   `endif  // BAR
//
// Rejected examples:
//   `ifdef FOO
//   `endif
//
//   `ifdef FOO
//   `endif  // BAR
//
class EndifCommentRule : public verible::TokenStreamLintRule {
 public:
  using rule_type = verible::TokenStreamLintRule;
  static absl::string_view Name();

  // Returns the description of the rule implemented formatted for either the
  // helper flag or markdown depending on the parameter type.
  static std::string GetDescription(DescriptionType);

  EndifCommentRule() : state_(State::kNormal) {}

  void HandleToken(const verible::TokenInfo& token) override;

  verible::LintRuleStatus Report() const override;

 private:
  // States of the internal token-based analysis.
  enum class State {
    kNormal,
    kExpectPPIdentifier,
    kExpectEndifComment,
  };

  // Link to style guide rule.
  static const char kTopic[];

  // Diagnosic message.
  static const char kMessage[];

  // Internal lexical analysis state.
  State state_;

  // Token information for the last seen `endif.
  verible::TokenInfo last_endif_ = verible::TokenInfo::EOFToken();

  // Stack of nested preprocessor conditionals.
  std::stack<verible::TokenInfo> conditional_scopes_;

  // Collection of found violations.
  std::set<verible::LintViolation> violations_;
};

}  // namespace analysis
}  // namespace verilog

#endif  // VERIBLE_VERILOG_ANALYSIS_CHECKERS_ENDIF_COMMENT_RULE_H_
