#!/usr/bin/env bash

function inc_total_test_count() {
	if [ -n ${TEST_TOTAL_COUNT+x} ]
	then
		((TEST_TOTAL_COUNT=TEST_TOTAL_COUNT + 1))
	fi
}

function inc_success_test_count() {
	if [ -n ${TEST_SUCCESS_COUNT+x} ]
	then
		((TEST_SUCCESS_COUNT=TEST_SUCCESS_COUNT + 1))
	fi
}

function inc_skipped_test_count() {
	if [ -n ${TEST_SKIPPED_COUNT+x} ]
	then
		((TEST_SKIPPED_COUNT=TEST_SKIPPED_COUNT + 1))
	fi
}

function assert_compilation_error() {
	if [ "$2" = "SKIP" ]
	then
		inc_skipped_test_count
		return
	fi
	inc_total_test_count
	set +e
	./build/zephyr -c "$1" -o ./build/test >/dev/null 2>&1
	res=$?
	set -e

	if [ $res -eq 0 ]
	then
		echo ""
		echo "======================================="
		echo "              Test failed              "
		echo $1
		echo "   Expected failure whilst compiling   "
		echo "======================================="
		return
	fi

	echo -n "="
	inc_success_test_count
}

function assert_exit_code() {
	if [ "$3" = "SKIP" ]
	then
		inc_skipped_test_count
		return
	fi

	inc_total_test_count
	set +e
	./build/zephyr -c "$2" -o ./build/test
	res=$?
	set -e

	if [ $res -ne 0 ]
	then
		echo ""
		echo "======================================="
		echo "              Test failed              "
		echo "- - - - - - - - - - - - - - - - - - - -"
		echo $2
		echo "- - - - - - - - - - - - - - - - - - - -"
		echo "        Failure while compiling        "
		echo "======================================="
		return
	fi

	set +e
	./build/test >/dev/null 2>&1
	res=$?
	set -e

	if [ $res -ne $1 ]
	then
		echo ""
		echo "======================================="
		echo "              Test failed              "
		echo "- - - - - - - - - - - - - - - - - - - -"
		echo $2
		echo "- - - - - - - - - - - - - - - - - - - -"
		echo "Expected exit code" $1 "but got" $res
		echo "======================================="
		return
	fi

	echo -n "="
	inc_success_test_count
}

function assert_stdout() {
	if [ "$3" = "SKIP" ]
	then
		inc_skipped_test_count
		return
	fi
	inc_total_test_count
	set +e
	./build/zephyr -c "$2" -o ./build/test
	res=$?
	set -e

	if [ $res -ne 0 ]
	then
		echo ""
		echo "======================================="
		echo "              Test failed              "
		echo "- - - - - - - - - - - - - - - - - - - -"
		echo $2
		echo "- - - - - - - - - - - - - - - - - - - -"
		echo "        Failure while compiling        "
		echo "======================================="
		return
	fi

	set +e
	output=$(./build/test)
	res=$?
	set -e

	if [ $res -ne 0 ]
	then
		echo ""
		echo "======================================="
		echo "              Test failed              "
		echo "- - - - - - - - - - - - - - - - - - - -"
		echo $2
		echo "- - - - - - - - - - - - - - - - - - - -"
		echo "Exited with a non-zero exit code" $res
		echo "======================================="
		return
	fi

	if [[ "$output" != $1 ]]
	then
		echo ""
		echo "======================================="
		echo "              Test failed              "
		echo "- - - - - - - - - - - - - - - - - - - -"
		echo $2
		echo "- - - - - - - - - - - - - - - - - - - -"
		echo "            Expected stdout            "
		echo "- - - - - - - - - - - - - - - - - - - -"
		echo $1
		echo "- - - - - - - - - - - - - - - - - - - -"
		echo "            Received stdout            "
		echo "- - - - - - - - - - - - - - - - - - - -"
		echo $output
		echo "- - - - - - - - - - - - - - - - - - - -"
		echo "======================================="
		return
	fi

	echo -n "="
	inc_success_test_count
}

function assert_stdout_argv() {
	inc_total_test_count
	set +e
	./build/zephyr -c "$2" -o ./build/test
	res=$?
	set -e

	if [ $res -ne 0 ]
	then
		echo ""
		echo "======================================="
		echo "              Test failed              "
		echo "- - - - - - - - - - - - - - - - - - - -"
		echo $2
		echo "- - - - - - - - - - - - - - - - - - - -"
		echo "        Failure while compiling        "
		echo "======================================="
		return
	fi

	set +e
	output=$(./build/test "${@:3}")
	res=$?
	set -e

	if [ $res -ne 0 ]
	then
		echo ""
		echo "======================================="
		echo "              Test failed              "
		echo "- - - - - - - - - - - - - - - - - - - -"
		echo $2
		echo "- - - - - - - - - - - - - - - - - - - -"
		echo "Exited with a non-zero exit code" $res
		echo "======================================="
		return
	fi

	if [[ "$output" != $1 ]]
	then
		echo ""
		echo "======================================="
		echo "              Test failed              "
		echo "- - - - - - - - - - - - - - - - - - - -"
		echo $2
		echo "- - - - - - - - - - - - - - - - - - - -"
		echo "            Expected stdout            "
		echo "- - - - - - - - - - - - - - - - - - - -"
		echo $1
		echo "- - - - - - - - - - - - - - - - - - - -"
		echo "            Received stdout            "
		echo "- - - - - - - - - - - - - - - - - - - -"
		echo $output
		echo "- - - - - - - - - - - - - - - - - - - -"
		echo "======================================="
		return
	fi

	echo -n "="
	inc_success_test_count
}