#!/usr/bin/env bash

function assert_exit_code() {
	./build/zephyr -c "$1" -o ./build/test
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
	fi

	echo -n "="
}