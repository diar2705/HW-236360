
#!/bin/bash

# Compile the project
make

# Exit if compilation fails
if [ $? -ne 0 ]; then
    echo "❌ Compilation failed."
    exit 1
fi

# Folders containing test files
TEST_DIRS=("hw3-tests" "tests" "tests2")

# Counters
PASS=0
FAIL=0

echo "========================="
echo "     Running All Tests"
echo "========================="

for DIR in "${TEST_DIRS[@]}"; do
    echo ""
    echo "▶ Directory: $DIR"
    
    for IN_FILE in "$DIR"/*.in; do
        BASENAME=$(basename "$IN_FILE" .in)
        OUT_FILE="$DIR/$BASENAME.out"
        RES_FILE="$DIR/$BASENAME.res"

        # Run program and save output to .res file
        ./hw3 < "$IN_FILE" > "$RES_FILE"

        # Compare result
        if diff -q "$RES_FILE" "$OUT_FILE" > /dev/null; then
            echo -e "  ✅ $BASENAME"
            ((PASS++))
        else
            echo -e "  ❌ $BASENAME"
            ((FAIL++))
        fi
    done
done

# Final summary
echo ""
echo "========================="
echo "         Summary"
echo "========================="
echo -e "✅ Passed: $PASS"
echo -e "❌ Failed: $FAIL"
echo "========================="

