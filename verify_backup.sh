#!/bin/bash

# ==============================================================================
# Script Name: verify_backup.sh
# Description: Compares file counts and total size between Source and Destination.
#              Filters out macOS metadata (._ files, .DS_Store) for accuracy.
# Usage:       ./verify_backup.sh "Source Path" "Destination Path"
# ==============================================================================

SOURCE_DIR="$1"
DEST_DIR="$2"

# 1. Validation check
if [ -z "$SOURCE_DIR" ] || [ -z "$DEST_DIR" ]; then
    echo "Usage: ./verify_backup.sh <source_path> <destination_path>"
    exit 1
fi

echo "--- Backup Verification Report ---"
echo "Source:      $SOURCE_DIR"
echo "Destination: $DEST_DIR"
echo "----------------------------------"

# 2. Function to count valid files (ignores .DS_Store and hidden ._ files)
#    find: searches recursively
#    -type f: files only
#    grep -v: excludes patterns
#    wc -l: counts lines
count_files() {
    find "$1" -type f | grep -v "\.DS_Store" | grep -v "/\._" | wc -l | xargs
}

# 3. Function to calculate total size in human readable format
#    du -sh: disk usage, summary, human-readable
get_size() {
    du -sh "$1" | awk '{print $1}'
}

echo "Analyzing Source..."
SRC_COUNT=$(count_files "$SOURCE_DIR")
SRC_SIZE=$(get_size "$SOURCE_DIR")

echo "Analyzing Destination..."
DEST_COUNT=$(count_files "$DEST_DIR")
DEST_SIZE=$(get_size "$DEST_DIR")

echo ""
echo "--- Results ---"
echo "Source Files:      $SRC_COUNT"
echo "Destination Files: $DEST_COUNT"
echo ""
echo "Source Size:       $SRC_SIZE"
echo "Destination Size:  $DEST_SIZE"
echo "----------------------------------"

# 4. Logic Analysis
if [ "$SRC_COUNT" -eq "$DEST_COUNT" ]; then
    echo "SUCCESS: File counts match exactly."
elif [ "$DEST_COUNT" -lt "$SRC_COUNT" ]; then
    DIFF=$((SRC_COUNT - DEST_COUNT))
    echo "WARNING: Destination has $DIFF fewer files."
    echo "Reason: This may happen if the Organizer skipped duplicate files or if system files (.Trashes, etc.) were filtered out."
else
    DIFF=$((DEST_COUNT - SRC_COUNT))
    echo "WARNING: Destination has $DIFF more files."
    echo "Reason: The destination folder might have contained data prior to this backup."
fi