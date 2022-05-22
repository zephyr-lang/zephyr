#!/usr/bin/env bash

. tests/asserts.sh

echo -n "Ternary Expression: "
assert_exit_code 2 "function main(): int { return 0 ? 10 : 2; }"
assert_exit_code 10 "function main(): int { return 1 ? 10 : 2; }"
assert_compilation_error "function x() {} function main: int { return x() ? 1 : 2; }"
echo " Done"

echo -n "If Statements: "
assert_exit_code 5 "
function main(): int {
	var a: int = 0;

	if(1) {
		a = 5;
	}

	return a;
}
"

assert_exit_code 12 "
function main(): int {
	var a: int = 0;

	if(0) {
		a = 5;
	}
	else {
		a = 12;
	}

	return a;
}
"

assert_exit_code 5 "
function main(): int {
	var a: int = 0;

	if(1) {
		a = 5;
	}
	else {
		a = 12;
	}

	return a;
}
"

assert_compilation_error "
function x() {}
function main(): int {
	if(x()) {}

	return 0;
}
"

echo " Done"

echo -n "While Statements: "
assert_stdout "0
1
2
3
4
5
6
7
8
9" "
function main(): int {
	var i: int = 0;

	while(i < 10) {
		printu(i);
		i = i + 1;
	}

	return i - 10;
}
"

assert_compilation_error "function x() {} function main():int { while(x()) {} }"

assert_exit_code 5 "
function main(): int {
	var a: int = 5;

	while(0) a = 12;

	return a;
}
"

echo " Done"

echo -n "For Statements: "

assert_stdout "0
1
2
3
4
5
6
7
8
9" "
function main(): int {
	for(var i: int = 0; i < 10; i = i + 1) {
		printu(i);
	}
	return 0;
}
"

assert_exit_code 45 "
function main(): int {
	var k: int = 0;
	var i: int = 0;
	for(;i < 10; i = i + 1) k = k + i;
	return k;
}
"

assert_exit_code 45 "
function main(): int {
	var k: int = 0;
	var i: int = 0;
	for(;i < 10;) {
		k = k + i;
		i = i + 1;
	}
	return k;
}
"

assert_stdout "0
1
2
3
4
5
6
7
8
9
10" "
function main(): int {
	for(var i: int = 0;; i = i + 1) {
		printu(i);

		if(i == 10) {
			return 0;
		}
	}
	return 0;
}
"

echo " Done"

echo -n "SC Logical: "

assert_stdout "zz0
zo1
o1
o1" "
function z(): int { syscall3(1, 1, \"z\", 1); return 0; }
function o(): int { syscall3(1, 1, \"o\", 1); return 1; }

function main(): int {
	printu(z() || z());
	printu(z() || o());
	printu(o() || z());
	printu(o() || o());

	return 0;
}
"

assert_stdout "z0
z0
oz0
oo1" "
function z(): int { syscall3(1, 1, \"z\", 1); return 0; }
function o(): int { syscall3(1, 1, \"o\", 1); return 1; }

function main(): int {
	printu(z() && z());
	printu(z() && o());
	printu(o() && z());
	printu(o() && o());

	return 0;
}
"

echo " Done"

echo -n "Continue: "

assert_stdout "0
1
2
4
5
6
7
8
9" "
function main(): int {

	for(var i = 0; i < 10; i = i + 1) {
		if(i == 3) continue;
		printu(i);
	}

	return 0;
}
"

assert_stdout "1
2
4
5
6
7
8
9
10" "
function main(): int {

	for(var i = 0; i < 10;) {
		i = i + 1;
		if(i == 3) continue;
		printu(i);
	}

	return 0;
}
"

assert_stdout "1
2
3
4
6
7
8
9
10" "
function main(): int {

	var i = 0;

	while(i < 10) {
		i = i + 1;
		if(i == 5) continue;
		printu(i);
	}

	return 0;
}
"

echo " Done"

echo -n "Break: "

assert_stdout "0
1
2
3
4" "
function main(): int {

	for(var i = 0 ; i < 10; i = i + 1) {
		if(i == 5) break;
		printu(i);
	}

	return 0;
}
"

assert_stdout "0
1
2
3
4
5
6
7" "
function main(): int {

	var i = 0;
	while(i < 10) {
		if(i == 8) break;
		printu(i);

		i = i + 1;
	}

	return 0;
}
"

echo " Done"

echo -n "When: "

assert_stdout "0!
1!
Neither 0 nor 1!" "
import \"std/io.zpr\";

function main(): int {

	for(var i = 0; i < 3; ++i) {
		when(i) {
			0 -> putsln(\"0!\");
			1 -> putsln(\"1!\");
			else -> putsln(\"Neither 0 nor 1!\");
		}
	}

	return 0;
}
"

assert_stdout "0!
1!" "
import \"std/io.zpr\";

function main(): int {

	for(var i = 0; i < 3; ++i) {
		when(i) {
			0 -> putsln(\"0!\");
			1 -> putsln(\"1!\");
		}
	}

	return 0;
}
"

assert_stdout "not 1 or 2
Either 1 or 2
Either 1 or 2
not 1 or 2" "
import \"std/io.zpr\";

function main(): int {

	for(var i = 0; i <= 3; ++i) {
		when(i) {
			1, 2 -> putsln(\"Either 1 or 2\");
			else -> {
				putsln(\"not 1 or 2\");
			}
		}
	}

	return 0;
}
"

echo " Done"