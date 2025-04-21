#!/bin/bash

EXEC=./hw1
INPUT_DIR=Tests/input
EXPECTED_DIR=Tests/expected
OUTPUT_DIR=Tests/output

fail_count=0
pass_count=0

mkdir -p "$OUTPUT_DIR"

for i in {14..57}; do
    testname="t$i"
    infile="$INPUT_DIR/$testname.in"
    expected="$EXPECTED_DIR/$testname.out"
    outfile="$OUTPUT_DIR/$testname.out"

    echo "🔧 Running $testname..."

    $EXEC < "$infile" > "$outfile" 2>&1

    if ! diff -q "$expected" "$outfile" > /dev/null; then
        echo "❌ $testname failed:"
        diff "$expected" "$outfile"
        echo ""
        ((fail_count++))
    else
        echo "✅ $testname passed"
        ((pass_count++))
    fi
done

echo "==============================="
echo "✅ Passed: $pass_count"
echo "❌ Failed: $fail_count"
echo "==============================="

