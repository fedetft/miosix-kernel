#!/bin/sh

# This simple script will download all the required source files
# for compiling llvm

REPO_URL="https://github.com/llvm/llvm-project.git"
COMMIT_SHA="3b5b5c1ec4a3095ab096dd780e84d7ab81f3d7ff"
CLONE_DIR="llvm-project"

git init "$CLONE_DIR"
cd "$CLONE_DIR" || exit 1
git remote add origin "$REPO_URL"
git fetch --depth 1 origin "$COMMIT_SHA"
git -c advice.detachedHead=false checkout "$COMMIT_SHA"
rm -rf .git
cd ..
