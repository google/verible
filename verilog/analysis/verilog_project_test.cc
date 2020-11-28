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

#include "verilog/analysis/verilog_project.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "common/text/text_structure.h"
#include "common/util/file_util.h"
#include "common/util/logging.h"
#include "common/util/range.h"
#include "verilog/CST/module.h"

namespace verilog {
namespace {

using verible::TextStructureView;
using verible::file::Basename;
using verible::file::CreateDir;
using verible::file::JoinPath;
using verible::file::testing::ScopedTestFile;

class TempDirFile : public ScopedTestFile {
 public:
  TempDirFile(absl::string_view content)
      : ScopedTestFile(::testing::TempDir(), content) {}
};

TEST(VerilogSourceFileTest, Initialization) {
  const VerilogSourceFile file("a.sv", "x/y/a.sv");
  // no attempt to open this file yet
  EXPECT_EQ(file.ReferencedPath(), "a.sv");
  EXPECT_EQ(file.ResolvedPath(), "x/y/a.sv");
  EXPECT_TRUE(file.Status().ok());
  EXPECT_EQ(file.GetTextStructure(), nullptr);
}

TEST(VerilogSourceFileTest, OpenExistingFile) {
  constexpr absl::string_view text("localparam int p = 1;\n");
  TempDirFile tf(text);
  const absl::string_view basename(Basename(tf.filename()));
  VerilogSourceFile file(basename, tf.filename());
  EXPECT_TRUE(file.Open().ok());
  EXPECT_TRUE(file.Status().ok());
  EXPECT_EQ(file.ReferencedPath(), basename);
  EXPECT_EQ(file.ResolvedPath(), tf.filename());
  const TextStructureView* text_structure =
      ABSL_DIE_IF_NULL(file.GetTextStructure());
  const absl::string_view owned_string_range(text_structure->Contents());
  EXPECT_EQ(owned_string_range, text);

  // Re-opening doesn't change anything
  EXPECT_TRUE(file.Open().ok());
  EXPECT_TRUE(file.Status().ok());
  EXPECT_EQ(file.GetTextStructure(), text_structure);
  EXPECT_TRUE(verible::BoundsEqual(file.GetTextStructure()->Contents(),
                                   owned_string_range));
}

TEST(VerilogSourceFileTest, NonExistingFile) {
  VerilogSourceFile file("aa.sv", "/does/not/exist/aa.sv");
  EXPECT_FALSE(file.Open().ok());
  EXPECT_FALSE(file.Status().ok());
  EXPECT_EQ(file.GetTextStructure(), nullptr);
  // Still not there.
  EXPECT_FALSE(file.Open().ok());
  EXPECT_FALSE(file.Status().ok());
  EXPECT_EQ(file.GetTextStructure(), nullptr);
}

TEST(VerilogSourceFileTest, ParseValidFile) {
  constexpr absl::string_view text("localparam int p = 1;\n");
  TempDirFile tf(text);
  const absl::string_view basename(Basename(tf.filename()));
  VerilogSourceFile file(basename, tf.filename());
  // Parse automatically opens.
  EXPECT_TRUE(file.Parse().ok());
  EXPECT_TRUE(file.Status().ok());
  const TextStructureView* text_structure =
      ABSL_DIE_IF_NULL(file.GetTextStructure());
  const absl::string_view owned_string_range(text_structure->Contents());
  EXPECT_EQ(owned_string_range, text);
  const auto* tokens = &text_structure->TokenStream();
  EXPECT_NE(tokens, nullptr);
  const auto* tree = &text_structure->SyntaxTree();
  EXPECT_NE(tree, nullptr);

  // Re-parsing doesn't change anything
  EXPECT_TRUE(file.Parse().ok());
  EXPECT_TRUE(file.Status().ok());
  EXPECT_EQ(file.GetTextStructure(), text_structure);
  EXPECT_EQ(&text_structure->TokenStream(), tokens);
  EXPECT_EQ(&text_structure->SyntaxTree(), tree);
}

TEST(VerilogSourceFileTest, ParseInvalidFile) {
  constexpr absl::string_view text("localparam 1 = p;\n");
  TempDirFile tf(text);
  const absl::string_view basename(Basename(tf.filename()));
  VerilogSourceFile file(basename, tf.filename());
  // Parse automatically opens.
  EXPECT_FALSE(file.Parse().ok());
  EXPECT_FALSE(file.Status().ok());
  const TextStructureView* text_structure =
      ABSL_DIE_IF_NULL(file.GetTextStructure());
  const absl::string_view owned_string_range(text_structure->Contents());
  EXPECT_EQ(owned_string_range, text);
  const auto* tokens = &text_structure->TokenStream();
  EXPECT_NE(tokens, nullptr);
  const auto* tree = &text_structure->SyntaxTree();
  EXPECT_NE(tree, nullptr);
  // but syntax tree may be empty, depends on error-recovery

  // Re-parsing doesn't change anything
  EXPECT_FALSE(file.Parse().ok());
  EXPECT_FALSE(file.Status().ok());
  EXPECT_EQ(file.GetTextStructure(), text_structure);
  EXPECT_EQ(&text_structure->TokenStream(), tokens);
  EXPECT_EQ(&text_structure->SyntaxTree(), tree);
}

TEST(VerilogProjectTest, Initialization) {
  const auto tempdir = ::testing::TempDir();
  VerilogProject project(tempdir, {tempdir});
  EXPECT_TRUE(project.GetErrorStatuses().empty());
}

TEST(VerilogProjectTest, NonexistentTranslationUnit) {
  const auto tempdir = ::testing::TempDir();
  VerilogProject project(tempdir, {tempdir});
  const auto status_or_file = project.OpenTranslationUnit("never-there.v");
  EXPECT_FALSE(status_or_file.ok());
  EXPECT_EQ(project.GetErrorStatuses().size(), 1);
}

TEST(VerilogProjectTest, NonexistentIncludeFile) {
  const auto tempdir = ::testing::TempDir();
  VerilogProject project(tempdir, {tempdir});
  const auto status_or_file = project.OpenIncludedFile("nope.svh");
  EXPECT_FALSE(status_or_file.ok());
  EXPECT_EQ(project.GetErrorStatuses().size(), 1);
}

TEST(VerilogProjectTest, NonexistentFileLookup) {
  const auto tempdir = ::testing::TempDir();
  VerilogProject project(tempdir, {tempdir});
  VerilogSourceFile* file = project.LookupRegisteredFile("never-there.v");
  EXPECT_EQ(file, nullptr);
  EXPECT_TRUE(project.GetErrorStatuses().empty());
}

TEST(VerilogProjectTest, ValidTranslationUnit) {
  const auto tempdir = ::testing::TempDir();
  const std::string sources_dir = JoinPath(tempdir, "srcs");
  const std::string includes_dir = JoinPath(tempdir, "includes");
  EXPECT_TRUE(CreateDir(sources_dir).ok());
  EXPECT_TRUE(CreateDir(includes_dir).ok());
  VerilogProject project(sources_dir, {includes_dir});

  constexpr absl::string_view text("module m;\nendmodule\n");
  const ScopedTestFile tf(sources_dir, text);
  const auto status_or_file =
      project.OpenTranslationUnit(Basename(tf.filename()));
  VerilogSourceFile* verilog_source_file = *status_or_file;
  EXPECT_TRUE(verilog_source_file->Status().ok());
  EXPECT_EQ(verilog_source_file->ReferencedPath(), Basename(tf.filename()));
  EXPECT_EQ(verilog_source_file->ResolvedPath(), tf.filename());
  EXPECT_EQ(project.LookupRegisteredFile(Basename(tf.filename())),
            verilog_source_file);
  EXPECT_TRUE(project.GetErrorStatuses().empty());

  EXPECT_TRUE(verilog_source_file->Parse().ok());
  const auto* tree = ABSL_DIE_IF_NULL(
      verilog_source_file->GetTextStructure()->SyntaxTree().get());
  EXPECT_EQ(FindAllModuleDeclarations(*tree).size(), 1);

  {
    // Re-parsing the file changes nothing.
    EXPECT_TRUE(verilog_source_file->Parse().ok());
    const auto* tree2 = ABSL_DIE_IF_NULL(
        verilog_source_file->GetTextStructure()->SyntaxTree().get());
    EXPECT_EQ(tree2, tree);
    EXPECT_EQ(FindAllModuleDeclarations(*tree).size(), 1);
  }
  {  // Re-opening the file changes nothing.
    const auto status_or_file2 =
        project.OpenTranslationUnit(Basename(tf.filename()));
    VerilogSourceFile* verilog_source_file2 = *status_or_file2;
    EXPECT_EQ(verilog_source_file2, verilog_source_file);
    EXPECT_TRUE(verilog_source_file2->Status().ok());
  }

  // Testing begin/end iteration.
  for (auto& file : project) {
    EXPECT_TRUE(file.second.Parse().ok());
  }
  for (const auto& file : project) {
    EXPECT_TRUE(file.second.Status().ok());
  }
}

TEST(VerilogProjectTest, ValidIncludeFile) {
  const auto tempdir = ::testing::TempDir();
  const std::string sources_dir = JoinPath(tempdir, "srcs");
  const std::string includes_dir = JoinPath(tempdir, "includes");
  EXPECT_TRUE(CreateDir(sources_dir).ok());
  EXPECT_TRUE(CreateDir(includes_dir).ok());
  VerilogProject project(sources_dir, {includes_dir});

  constexpr absl::string_view text("`define FOO 1\n");
  const ScopedTestFile tf(includes_dir, text);
  const absl::string_view basename(Basename(tf.filename()));
  const auto status_or_file = project.OpenIncludedFile(basename);
  VerilogSourceFile* verilog_source_file = *status_or_file;
  EXPECT_TRUE(verilog_source_file->Status().ok());
  EXPECT_EQ(verilog_source_file->ReferencedPath(), basename);
  EXPECT_EQ(verilog_source_file->ResolvedPath(), tf.filename());
  EXPECT_EQ(project.LookupRegisteredFile(Basename(tf.filename())),
            verilog_source_file);
  EXPECT_TRUE(project.GetErrorStatuses().empty());

  // Re-opening same file, changes nothing
  {
    const auto status_or_file2 = project.OpenIncludedFile(basename);
    VerilogSourceFile* verilog_source_file2 = *status_or_file2;
    EXPECT_EQ(verilog_source_file2, verilog_source_file);
    EXPECT_TRUE(verilog_source_file2->Status().ok());
  }

  // includes aren't required to be parse-able, so just open
  EXPECT_TRUE(verilog_source_file->Open().ok());
  EXPECT_EQ(verilog_source_file->GetTextStructure()->SyntaxTree().get(),
            nullptr);

  // re-opening the file changes nothing
  EXPECT_TRUE(verilog_source_file->Open().ok());
  EXPECT_EQ(verilog_source_file->GetTextStructure()->SyntaxTree().get(),
            nullptr);
}

TEST(VerilogProjectTest, TranslationUnitNotFound) {
  const auto tempdir = ::testing::TempDir();
  const std::string sources_dir = JoinPath(tempdir, "srcs");
  const std::string includes_dir = JoinPath(tempdir, "includes");
  EXPECT_TRUE(CreateDir(sources_dir).ok());
  EXPECT_TRUE(CreateDir(includes_dir).ok());
  VerilogProject project(sources_dir, {includes_dir});

  constexpr absl::string_view text("module m;\nendmodule\n");
  // deliberately plant this file in the includes dir != sources dir
  const ScopedTestFile tf(includes_dir, text);
  {
    const auto status_or_file =
        project.OpenTranslationUnit(Basename(tf.filename()));
    EXPECT_FALSE(status_or_file.ok());
  }
  {  // try again, still fail
    const auto status_or_file =
        project.OpenTranslationUnit(Basename(tf.filename()));
    EXPECT_FALSE(status_or_file.ok());
  }
  {
    const auto statuses = project.GetErrorStatuses();
    EXPECT_EQ(statuses.size(), 1);
    for (const auto& status : statuses) {
      EXPECT_FALSE(status.ok());
    }
  }
}

TEST(VerilogProjectTest, IncludeFileNotFound) {
  const auto tempdir = ::testing::TempDir();
  const std::string sources_dir = JoinPath(tempdir, "srcs");
  const std::string includes_dir = JoinPath(tempdir, "includes");
  EXPECT_TRUE(CreateDir(sources_dir).ok());
  EXPECT_TRUE(CreateDir(includes_dir).ok());
  VerilogProject project(sources_dir, {includes_dir});

  constexpr absl::string_view text("module m;\nendmodule\n");
  // deliberately plant this file in the sources dir != include dir
  const ScopedTestFile tf(sources_dir, text);
  {
    const auto status_or_file =
        project.OpenIncludedFile(Basename(tf.filename()));
    EXPECT_FALSE(status_or_file.ok());
  }
  {  // try again, still fail
    const auto status_or_file =
        project.OpenIncludedFile(Basename(tf.filename()));
    EXPECT_FALSE(status_or_file.ok());
  }
  EXPECT_EQ(project.GetErrorStatuses().size(), 1);
}

}  // namespace
}  // namespace verilog
