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
	./build/zephyr -c "$1" -o ./build/test
	res=$?
	set -e

	if [ $res -ne 0 ]
	then
		echo "======================================="
		echo "              Test failed              "
		echo $1
		echo "        Failure while compiling        "
		echo "======================================="
		return
	fi

	set +e
	./build/test
	res=$?
	set -e

	if [ $res -ne $2 ]
	then
		echo ""
		echo "======================================="
		echo "              Test failed              "
		echo $1
		echo "Expected exit code" $2 "but got" $res
		echo "======================================="
		return
	fi

	echo -n "="
}