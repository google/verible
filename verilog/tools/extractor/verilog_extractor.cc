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

// verilog_extractor is a simple command-line utility to extract indexing facts
// from the given file.
//
// Example usage:
// verilog_extractor files...

#include <algorithm>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <sstream>  // IWYU pragma: keep  // for ostringstream
#include <string>   // for string, allocator, etc
#include <utility>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"  // for MakeArraySlice
#include "common/text/concrete_syntax_tree.h"
#include "common/text/symbol.h"
#include "common/text/parser_verifier.h"
#include "common/text/text_structure.h"
#include "common/text/token_info.h"
#include "common/util/file_util.h"
#include "common/util/init_command_line.h"
#include "common/util/logging.h"  // for operator<<, LOG, LogMessage, etc
#include "verilog/CST/verilog_tree_print.h"
#include "verilog/analysis/verilog_analyzer.h"
#include "verilog/parser/verilog_parser.h"
#include "verilog/CST/verilog_nonterminals.h"
#include "common/text/concrete_syntax_leaf.h"
#include "verilog/tools/extractor/verilog_extractor_types.h"


ABSL_FLAG(bool, printextraction,

          false, "Whether or not to print the extracted facts");

class Anchor {
public:
    Anchor(int startLocation, int endLocation, absl::string_view value) : startLocation_(startLocation),
                                                                          endLocation_(endLocation),
                                                                          value_(value) {};

    void print() {
        std::cout <<
                  "Anchor: {\n"
                  "StartLocation: " << startLocation_ << ",\n" <<
                  "EndLocation: " << endLocation_ << ",\n" <<
                  "Value: " << value_ << ",\n" <<
                  "}"
                  << std::endl;
    }

private:
    int startLocation_, endLocation_;
    std::string value_;
};

class Block {
public:
    Block(Type datatype = Type::NoType) : dataType_(datatype) {}

    Block(std::vector<Anchor> anchor, Type dataType) :
            anchors_(std::move(anchor)), dataType_(dataType) {}

    void print() {
        std::cout << "{\n";

        if (dataType_ != Type::NoType) {
            std::cout << "Type: " << dataType_ << ",\n";
        }

        for (auto anchor : anchors_) {
            anchor.print();
        }
        for (auto child : children_) {
            child.print();
        }

        std::cout << "}\n";
    }

    std::vector<Block> Children() { return children_; };

    void AppendChild(Block entry) { children_.push_back(std::move(entry)); };

    void AppendAnchor(Anchor entry) { anchors_.push_back(std::move(entry)); };

private:
    std::vector<Anchor> anchors_;

    Type dataType_;

    std::vector<Block> children_;
};

using TagResolver = std::function<Block(const verible::SyntaxTreeNode &, const verible::SyntaxTreeNode &,
                                        absl::string_view)>;
std::map<int, TagResolver> tagResolver;

const verible::Symbol *GetChildByTag(const verible::SyntaxTreeNode &root, verilog::NodeEnum tag) {
    for (const auto &child : root.children()) {
        if (child && child->Tag().tag == static_cast<int>(tag)) {
            return child.get();
        }
    }
    return nullptr;
}

void InitializeTagResolver() {
    tagResolver[static_cast<int>(verilog::NodeEnum::kModuleDeclaration)] = [=](const verible::SyntaxTreeNode &node,
                                                                               const verible::SyntaxTreeNode &root,
                                                                               absl::string_view base) {

        Block moduleBlock = Block(Type::Module);

        auto moduleHeader = GetChildByTag(node, verilog::NodeEnum::kModuleHeader);
        moduleBlock.AppendChild(tagResolver[static_cast<int>(verilog::NodeEnum::kModuleHeader)](
                verible::SymbolCastToNode(*moduleHeader), node,
                base));

        auto moduleItemList = GetChildByTag(node, verilog::NodeEnum::kModuleItemList);
        moduleBlock.AppendChild(tagResolver[static_cast<int>(verilog::NodeEnum::kModuleItemList)](
                verible::SymbolCastToNode(*moduleItemList),
                node, base));

        auto moduleEnd = GetChildByTag(node, verilog::NodeEnum::kLabel);
        if (moduleEnd != nullptr) {
            moduleBlock.AppendChild(
                    tagResolver[static_cast<int>(verilog::NodeEnum::kLabel)](verible::SymbolCastToNode(*moduleEnd),
                                                                             node, base));
        }

        return moduleBlock;
    };


    tagResolver[static_cast<int>(verilog::NodeEnum::kModuleHeader)] = [=](const verible::SyntaxTreeNode &node,
                                                                          const verible::SyntaxTreeNode &root,
                                                                          absl::string_view base) {
        Block moduleHeader = Block(Type::ModuleHeader);

        auto moduleKeyword = verible::SymbolCastToLeaf(*GetChildByTag(node, verilog::NodeEnum::kTimeLiteral));
        Anchor moduleKeywordAnchor = Anchor(moduleKeyword.get().left(base), moduleKeyword.get().right(base),
                                            moduleKeyword.get().text());
        moduleHeader.AppendAnchor(moduleKeywordAnchor);

        auto moduleName = verible::SymbolCastToLeaf(
                *GetChildByTag(node, verilog::NodeEnum::kNetVariableDeclarationAssign));
        Anchor moduleNameAnchor = Anchor(moduleName.get().left(base), moduleName.get().right(base),
                                         moduleName.get().text());
        moduleHeader.AppendAnchor(moduleNameAnchor);

        return moduleHeader;
    };

    tagResolver[static_cast<int>(verilog::NodeEnum::kModuleItemList)] = [=](const verible::SyntaxTreeNode &node,
                                                                            const verible::SyntaxTreeNode &root,
                                                                            absl::string_view base) {
        Block moduleBody = Block(Type::ModuleBody);
        return moduleBody;
    };

    tagResolver[static_cast<int>(verilog::NodeEnum::kLabel)] = [=](const verible::SyntaxTreeNode &node,
                                                                   const verible::SyntaxTreeNode &root,
                                                                   absl::string_view base) {
        Block moduleEnd = Block(Type::ModuleEnd);
        auto moduleEndKeyword = verible::SymbolCastToLeaf(
                *GetChildByTag(node, verilog::NodeEnum::kNetVariableDeclarationAssign));
        Anchor moduleNameAnchor = Anchor(moduleEndKeyword.get().left(base), moduleEndKeyword.get().right(base),
                                         moduleEndKeyword.get().text());
        moduleEnd.AppendAnchor(moduleNameAnchor);

        return moduleEnd;
    };
}

Block Extract(const verible::SyntaxTreeLeaf &node, absl::string_view base) {
    std::cout << "Start Leaf" << std::endl;
    std::cout << verilog::NodeEnumToString(static_cast<verilog::NodeEnum>(node.Tag().tag)) << " <<>> " << node.Tag().tag
              << " " << node.get()
              << std::endl;
    std::cout << "End Leaf" << std::endl;
    std::cout << std::endl;
}

Block Extract(const verible::SyntaxTreeNode &node, absl::string_view base) {
    std::cout << "Start Node" << std::endl;
    std::cout << verilog::NodeEnumToString(static_cast<verilog::NodeEnum>(node.Tag().tag)) << "  ";
    std::cout << node.children().size() << std::endl;

    auto resolver = tagResolver.find(node.Tag().tag);
    if (resolver != tagResolver.end()) {
        return resolver->second(node, node, base);
    }

    std::vector<Block> blocks;
    for (const auto &child : node.children()) {
        if (child) {
            if (child->Kind() == verible::SymbolKind::kNode) {
                return Extract(verible::SymbolCastToNode(*child), base);
            } else {
                return Extract(verible::SymbolCastToLeaf(*child), base);
            }
        }
    }

    std::cout << "End Node" << std::endl << std::endl;
}

static int ExtractOneFile(absl::string_view content,
                          absl::string_view filename) {
    int exit_status = 0;
    const auto analyzer =
            verilog::VerilogAnalyzer::AnalyzeAutomaticMode(content, filename);
    const auto lex_status = ABSL_DIE_IF_NULL(analyzer)->LexStatus();
    const auto parse_status = analyzer->ParseStatus();
    if (!lex_status.ok() || !parse_status.ok()) {
        const std::vector<std::string> syntax_error_messages(
                analyzer->LinterTokenErrorMessages());
        for (const auto &message : syntax_error_messages) {
            std::cout << message << std::endl;
        }
        exit_status = 1;
    }
    const bool parse_ok = parse_status.ok();

    const auto &text_structure = analyzer->Data();
    const auto &syntax_tree = text_structure.SyntaxTree();

    // check for printextraction flag, and print extraction if on
    if (absl::GetFlag(FLAGS_printextraction) && syntax_tree != nullptr) {
        std::cout << std::endl
                  << (!parse_ok ? " (incomplete due to syntax errors): " : "")
                  << std::endl;

        Block mainBlock = Block(Type::File);
        auto block = Extract(verible::SymbolCastToNode(*syntax_tree), analyzer->Data().Contents());
        mainBlock.AppendChild(block);
        mainBlock.print();
    }

    return exit_status;
}

int main(int argc, char **argv) {
    const auto usage =
            absl::StrCat("usage: ", argv[0], " [options] <file> [<file>...]");
    const auto args = verible::InitCommandLine(usage, &argc, &argv);

    InitializeTagResolver();

    int exit_status = 0;
    // All positional arguments are file names.  Exclude program name.
    for (const auto filename :
            verible::make_range(args.begin() + 1, args.end())) {
        std::string content;
        if (!verible::file::GetContents(filename, &content).ok()) {
            exit_status = 1;
            continue;
        }

        int file_status = ExtractOneFile(content, filename);
        exit_status = std::max(exit_status, file_status);
    }
    return exit_status;
}
