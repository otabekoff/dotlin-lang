#!/usr/bin/env bash

DOTLIN="./build/apps/dotlin"
TEST_DIR="/home/otabek/Projects/langs/cpp/dotlin/tests/files"
OUT="ALL_TEST_OUTPUTS.txt"

# Start fresh
echo "Dotlin test run - $(date)" > "$OUT"
echo "=========================" >> "$OUT"
echo >> "$OUT"

find "$TEST_DIR" -type f -name "*.lin" | sort | while read -r file; do
    echo "▶ Running: $file" | tee -a "$OUT"
    echo "----------------------------------------" >> "$OUT"

    "$DOTLIN" "$file" >> "$OUT" 2>&1

    echo >> "$OUT"
    echo >> "$OUT"
done

echo "DONE. All outputs saved in $OUT"
