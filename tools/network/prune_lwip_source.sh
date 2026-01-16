#!/usr/bin/env bash

# This script prunes the lwIP source code to only include the files
# necessary for Miosix. It removes unused files and directories
# (examples, docs, tests) to reduce the size of the source tree.
#
# Usage: ./prune_lwip_source.sh /path/to/lwip/source
# The script will modify the lwIP source tree in place.

# Check if the correct number of arguments is provided
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 /path/to/lwip/source"
    exit 1
fi

SOURCE_DIR="$1"

# Verify that the provided path is a directory
if [ ! -d "$SOURCE_DIR" ]; then
    echo "Error: $SOURCE_DIR is not a valid directory."
    exit 1
fi

# List of files and directories to keep, everything else will be removed
FILES_TO_KEEP=(
    "src"
    "CMakeLists.txt"
    "COPYING"
    "README"
)

# Navigate to the source directory
cd "$SOURCE_DIR" || exit 1

# Normalize SOURCE_DIR to an absolute, canonical path (we already cd'ed into it above)
SOURCE_DIR="$(pwd -P)"
echo "Pruning lwIP source in $SOURCE_DIR"

# Remove everything except the files and directories we want to keep
echo "The following items will be removed:"
TO_REMOVE=()
for item in "$SOURCE_DIR"/*; do
    if [[ ! " ${FILES_TO_KEEP[@]} " =~ " ${item} " ]]; then
        TO_REMOVE+=("$item")
        echo -e "\t$item"
    fi
done

# Ask for confirmation before deleting
read -p "Are you sure you want to proceed? (y/N): " confirm
if [[ "$confirm" != "y" ]]; then
    echo "Aborting."
    exit 0
fi

for item in "${TO_REMOVE[@]}"; do
    if [ -e "$item" ]; then
        rm -rf -- "$item"
    fi
done
echo "Done."
