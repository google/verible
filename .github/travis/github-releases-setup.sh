#!/bin/bash
# Copyright 2020 The Verible Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# GitHub Releases
# ---------------
# Generate the GitHub Releases to deploy

RELEASE_DIR=${1:-/tmp/releases}
rm -rf $RELEASE_DIR

GIT_VERSION=${GIT_VERSION:-$(git describe --match=v*)}

PREFIX=$RELEASE_DIR/verible-$GIT_VERSION
PREFIX_BIN=$PREFIX/bin
PREFIX_DOC=$PREFIX/share/verible
PREFIX_MAN=$PREFIX/share/man/man1
mkdir -p $PREFIX
mkdir -p $PREFIX_BIN
mkdir -p $PREFIX_DOC
mkdir -p $PREFIX_MAN

# Binaries
bazel run :install -c opt -- $PREFIX_BIN
for BIN in $PREFIX_BIN/*; do
    ls -l $BIN
    file $BIN
    ldd $BIN
done

# Documentation
cp -a /tmp/pages/* $PREFIX_DOC
# Man pages
for m in verilog_syntax verilog_lint verilog_format ; do
    gflags2man --help_flag="--helpfull" --dest_dir $PREFIX_MAN $PREFIX_BIN/$m
    gzip $PREFIX_MAN/$m.1
done

DISTRO_ARCH=$(uname -m)
DISTRO=$(lsb_release --short --id)
DISTRO_RELEASE=$(lsb_release --short --release)
DISTRO_CODENAME=$(lsb_release --short --codename)
TARBALL=$RELEASE_DIR/verible-$GIT_VERSION-$DISTRO-$DISTRO_RELEASE-$DISTRO_CODENAME-$DISTRO_ARCH.tar.gz
(
    cd $RELEASE_DIR
    tar -zcvf $TARBALL verible-$GIT_VERSION
)
