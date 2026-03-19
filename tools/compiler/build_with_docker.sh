#!/bin/bash
set -euo pipefail

# Script must be run from the compiler directory.
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

list_compilers() {
    find . -maxdepth 1 -type d -name "gcc-*" -printf "%f\n" | sort
}

# Select compiler
if [[ $# -ge 1 ]]; then
    COMPILER="$1"
else
    echo "Available compilers:"
    list_compilers
    echo ""
    read -p "Select compiler: " COMPILER
fi

# Validate
if [[ ! -d "$COMPILER" ]]; then
    echo "Error: directory '$COMPILER' does not exist."
    exit 1
fi

echo "Using compiler: $COMPILER"
echo ""

# Build using Dockerfile in this directory
docker build \
    --build-arg COMPILER="$COMPILER" \
    --output "$SCRIPT_DIR" \
    -t toolchain-builder:"$COMPILER" \
    "$SCRIPT_DIR"
