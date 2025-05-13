#!/bin/bash

# Compile the code
make clean
make

# Run the tests
for i in {1..3}; do
  echo "Testing t$i.in"
  ./hw2 < hw2-tests/t$i.in > hw2-tests/t$i.result
  diff hw2-tests/t$i.out hw2-tests/t$i.result
  if [ $? -eq 0 ]; then
    echo "Test $i passed!"
  else
    echo "Test $i failed!"
  fi
  echo "----------------------------"
done
