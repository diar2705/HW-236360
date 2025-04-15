#!/bin/bash

fail_count=0

for i in {1..11}
do
  ./hw1 < hw1-tests/t$i.in > hw1-tests/t$i.res 2>&1
  if ! diff -q hw1-tests/t$i.out hw1-tests/t$i.res > /dev/null; then
    echo "❌ Test $i failed:"
    diff hw1-tests/t$i.out hw1-tests/t$i.res
    echo ""
    ((fail_count++))
  fi
done

echo "=== $fail_count test(s) failed ==="

