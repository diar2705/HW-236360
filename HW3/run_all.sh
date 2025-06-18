#!/bin/bash


./clearTestResults

TOTAL_FAILED=0
ALL_FAILED_TESTS=()

# Process all subdirectories in allTests
for dir in allTests/*/; do
    dir_name=$(basename "$dir")
    echo "==============================================="
    echo "Testing directory: $dir_name"
    echo "==============================================="

    FAILED_TESTS=()

    # Run tests for each .in file in the directory
    for in_file in "$dir"/*.in; do
        # Skip if no .in files exist
        [ -e "$in_file" ] || continue

        test_name=$(basename "$in_file" .in)
        echo "Running test: $test_name"

        # Run hw3 and save output
        ./hw3 < "$in_file" > "$dir/$test_name.res" 2>&1

        # Compare with expected output
        diff -q "$dir/$test_name.res" "$dir/$test_name.out" > /dev/null
        if [ $? -ne 0 ]; then
            FAILED_TESTS+=("$test_name")
            ALL_FAILED_TESTS+=("$dir_name/$test_name")
            ((TOTAL_FAILED++))
        fi
    done

    # Report results for this directory
    if [ ${#FAILED_TESTS[@]} -eq 0 ]; then
        echo "All tests in $dir_name passed ✅"
    else
        echo "Failed tests in $dir_name ❌:"
        for test in "${FAILED_TESTS[@]}"; do
            echo "  $test"
        done
    fi
    echo ""
done

# Report overall results
echo "==============================================="
echo "SUMMARY"
echo "==============================================="
if [ $TOTAL_FAILED -eq 0 ]; then
    echo "All tests in all directories passed ✅"
else
    echo "$TOTAL_FAILED test(s) failed across all directories ❌"
    echo "Failed tests:"
    for test in "${ALL_FAILED_TESTS[@]}"; do
        echo "  $test"
    done
fi