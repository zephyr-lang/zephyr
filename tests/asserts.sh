#!/usr/bin/env bash

function assert_compilation_error() {
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
}

function assert_exit_code() {
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
	./build/test
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
}

function assert_stdout() {
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
}