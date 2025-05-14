#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

#	number of tests:
TESTS_DIR="AllTests"
HW_EXE="./hw2"

function notify() {
    local len=$((${#1}+2))
    let border=$len*2
	let spaceSeperator="$len/2"
	tmp="#%$spaceSeperator""s""%s""%$spaceSeperator""s#"
    printf "\n"
    printf "#%.0s" $(seq 1 $border)
	printf "\n"
    printf $tmp "" $1 ""
	printf "\n"
    printf "#%.0s" $(seq 1 $border)
	printf "\n\n"
}

test_failed=false
#	command to execute test:
for test_input in $TESTS_DIR/*.in; 
	do
		test_name=$(basename "$test_input" .in)
#		echo $test_name
		test_output="$TESTS_DIR/$test_name.out"
#		echo $test_output
		test_result="$TESTS_DIR/$test_name.res"
#		echo $test_result

		# Run the test
		$HW_EXE < "$test_input" > "$test_result"
		
		# Compare the results
		if diff $test_result $test_output > /dev/null; 
		then
			echo -e "Test ${test_name} - ${GREEN}PASSED${NC}"
			rm $test_result
		else
			echo -e "Test ${test_name} - ${RED}FAILED${NC}"
			diff $test_result $test_output
			test_failed=true
		fi
	done

if $test_failed
then
	notify "${RED}Unfortunatly!!! Something Failed TT${NC}"
else
	#notify "${GREEN}Congratz!!! All Tests have PASSED!!!${NC}"
	notify "Passed"
fi