#!/bin/bash

tmpdir="selfcheck_tmp"
if [ ! -f "submission.zip" ]
	then
		zip submission.zip main.cpp scanner.lex tokens.hpp output.hpp output.cpp
		
fi

if [ ! -f "submission.zip" ]
	then
		echo "Submission zip file not found!"

		exit
fi

rm -rf "$tmpdir" &> /dev/null
if [ -d "$tmpdir" ]
	then
		echo "Cannot clear tmp directory. Please delete '"$tmpdir"' manually and try again"
		exit
fi
mkdir "$tmpdir" &> /dev/null

unzip "submission.zip" -d "$tmpdir" &> /dev/null
if [[ $? != 0 ]] 
	then
		echo "Unable to unzip submission file!"
		exit
fi

cd "$tmpdir"
if [ ! -f scanner.lex ]
	then
		echo "File scanner.lex not found!"
		exit
fi

flex scanner.lex &> /dev/null
if [[ $? != 0 ]] 
	then
		echo "Cannot build submission [flex]!"
		exit
fi
g++ -std=c++17 -o hw1 *.c *.cpp &> /dev/null
if [[ $? != 0 ]] 
	then
		echo "Cannot build submission! [g++]"
		exit
fi
if [ ! -f hw1 ]
	then
		echo "Cannot build submission! [hw1]"
		exit
fi

RED="\e[31m"
GREEN="\e[32m"
ENDCOLOR="\e[0m"


failed=false
rm -rf ../Tests/output &> /dev/null
mkdir ../Tests/output
for test_input in ../Tests/input/*.in; 
	do
		test_name=$(basename "$test_input" .in)
		test_output="../Tests/expected/$test_name.out"
		test_result="../Tests/output/$test_name.res"

		# Run the test
		./hw1 < "$test_input" > "$test_result"
		
		# Compare the results
		if diff $test_result $test_output > /dev/null; 
		then
			echo -e "Test ${test_name} - ${GREEN}PASSED${NC}"
			rm $test_result
		else
			echo -e "Test ${test_name} - ${RED}FAILED${NC}"
			diff $test_result $test_output
			failed=true
		fi
	done

if $failed ; then
	cd - &> /dev/null
	rm -rf "$tmpdir"
    exit
fi

cd - &> /dev/null
rm -rf "$tmpdir"

echo -e "All tests ${GREEN}PASSED${ENDCOLOR}! Ok to submit :)"
exit
