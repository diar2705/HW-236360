#!/bin/bash

# Compile the project
make > /dev/null 2>&1

# Exit if compilation fails
if [ $? -ne 0 ]; then
    echo "❌ Compilation failed."
    exit 1
fi

TEST_DIRS=()
# get all sub-dirs of allTests
for dir in allTests/*/; do
    TEST_DIRS+=("$dir")
done

# Counters
PASS=0
FAIL=0

echo "========================="
echo "     Running All Tests"
echo "========================="

for DIR in "${TEST_DIRS[@]}"; do
    echo ""
    echo "▶ Directory: $DIR"
    
    dos2unix "$DIR"/*.in > /dev/null 2>&1
    dos2unix "$DIR"/*.out > /dev/null 2>&1

    for IN_FILE in "$DIR"/*.in; do
        BASENAME=$(basename "$IN_FILE" .in)
        OUT_FILE="$DIR/$BASENAME.out"
        LL_FILE="$DIR/$BASENAME.ll"
        RES_FILE="$DIR/$BASENAME.res"

        # Run program and save output to .res file
        ./hw5 < "$IN_FILE" 2>&1 > "$LL_FILE"
        lli "$LL_FILE" > "$RES_FILE" 2> /dev/null

        # Compare result
        if diff -q "$RES_FILE" "$OUT_FILE" > /dev/null; then
            echo -e "  ✅ $BASENAME"
            rm "$RES_FILE" "$LL_FILE" > /dev/null
            ((PASS++))
        else
            echo -e "  ❌ $BASENAME"
            ((FAIL++))
        fi
    done
done

make clean > /dev/null

# show differences for first NUM failed tests
NUM=$1
if [ -n "$NUM" ] && [ $FAIL -gt 0 ]; then
    echo ""
    echo "========================="
    echo "     Failed Tests"
    echo "========================="
    echo ""
    MIN=$(( NUM < FAIL ? NUM : FAIL ))
    echo "Showing differences for the first $MIN failed tests:"
    echo ""

    count=0
    for DIR in "${TEST_DIRS[@]}"; do
        for IN_FILE in "$DIR"/*.in; do
            BASENAME=$(basename "$IN_FILE" .in)
            RES_FILE="$DIR/$BASENAME.res"
            OUT_FILE="$DIR/$BASENAME.out"

            if [ -f "$RES_FILE" ] && [ -f "$OUT_FILE" ]; then
                if ! diff -q "$OUT_FILE" "$RES_FILE" > /dev/null; then
                    echo ""
                    echo "Failed Test: $BASENAME"
                    echo "Expected:"
                    cat -A $OUT_FILE
                    echo ""
                    echo "Got:"
                    cat -A $RES_FILE
                    echo "========================="
                    ((count++))
                    if [ $count -ge $NUM ]; then
                        break 2
                    fi
                fi
            fi
        done
    done
fi

# Final summary
echo ""
echo "========================="
echo "         Summary"
echo "========================="
echo -e "✅ Passed: $PASS"
echo -e "❌ Failed: $FAIL"
echo "========================="

# create submission file
if [ $FAIL -eq 0 ]; then
    if [ -f submission.zip ]; then
        rm submission.zip > /dev/null
    fi
    zip -r submission.zip \
        analyzer.cpp analyzer.hpp \
        main.cpp \
        nodes.cpp nodes.hpp \
        output.cpp output.hpp \
        parser.y scanner.lex \
        symbolTable.cpp symbolTable.hpp \
        visitor.hpp > /dev/null
    echo "🎉 All tests passed! Submission file created: submission.zip"
fi

