#!/usr/bin/env bash

. tests/asserts.sh

echo -n "General: "

assert_compilation_error "
function x(i: i32[4]) {}
"

assert_compilation_error "
function x(): i64[2] {}
"

assert_compilation_error "
var x: i8[4] = [1,2,3,4,5];
"

assert_compilation_error "
function x() {}
var x: i8[2] = [x(), x()];
"

echo " Done"

echo -n "Sizeof: "

assert_stdout "8
16
5
4" "
function main(): int {
	printu(sizeof(i64[1]));
	printu(sizeof(i32[4]));
	printu(sizeof(i8[5]));
	printu(sizeof(i16[2]));
	return 0;
}
"

echo " Done"

echo -n "Local: "
assert_stdout "1" "
function main(): int {
	var x: int[4] = [1, 2, 3, 4];
	printu(x[0]);
	return 0;
}
"

assert_stdout "5
6
7
8" "
function main(): int {
	var x: i32[4] = [5,6,7,8];

	for(var i = 0; i < 4; i = i + 1) {
		printu(x[i]);
	}
	return 0;
}
"
assert_stdout "6
7
3
4" "
function main(): int {
	var x: i32[4] = [1,2,3,4];
	x[0] = 6;
	x[1] = 7;

	for(var i = 0; i < 4; i = i + 1) {
		printu(x[i]);
	}
	return 0;
}
"

assert_stdout "2" "function s(x: int*): int {
	return x[0];
}

function main(): int {
	var x: int[4] = [1, 2, 3, 4];
	printu(s(&x[1]));

	return 0;
}"

echo " Done"

echo -n "Global: "
assert_stdout "1" "
var x: int[4] = [1, 2, 3, 4];
function main(): int {
	printu(x[0]);
	return 0;
}
"

assert_stdout "5
6
7
8" "
var x: i32[4] = [5,6,7,8];
function main(): int {
	for(var i = 0; i < 4; i = i + 1) {
		printu(x[i]);
	}
	return 0;
}
"
assert_stdout "6
7
3
4" "
var x: i32[4] = [1,2,3,4];
function main(): int {
	x[0] = 6;
	x[1] = 7;

	for(var i = 0; i < 4; i = i + 1) {
		printu(x[i]);
	}
	return 0;
}
"

assert_stdout "2" "function s(x: int*): int {
	return x[0];
}

var x: int[4] = [1, 2, 3, 4];

function main(): int {
	printu(s(&x[1]));

	return 0;
}"

echo " Done"