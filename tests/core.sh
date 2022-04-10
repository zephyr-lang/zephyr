#!/usr/bin/env bash

. tests/asserts.sh

echo -n "Basic returns: "
assert_exit_code 0 "function main(): int { return 0; }"
assert_exit_code 100 "function main(): int { return 100; }"
echo " Done"

echo -n "Unary: "
assert_exit_code 255 "function main(): int { return -1; }" # Unsigned
assert_exit_code 0 "function main(): int { return !56; }"
assert_exit_code 1 "function main(): int { return !0; }"
assert_exit_code 253 "function main(): int { return ~2; }" # Unsigned
assert_exit_code 0 "function main(): int { return !~0; }"
echo " Done"

echo -n "Arithmetic: "
assert_exit_code 8 "function main():int { return 5 + 3; }"
assert_exit_code 3 "function main():int { return 12 - 9; }"
assert_exit_code 56 "function main():int { return 8 * 7; }"
assert_exit_code 12 "function main():int { return 37 / 3; }"
assert_exit_code 1 "function main():int { return 37 % 3; }"
assert_exit_code 20 "function main():int { return 5 + 3 * 5; }"
assert_exit_code 40 "function main():int { return (5 + 3) * 5; }"
echo " Done"

echo -n "Bitwise: "
assert_exit_code 2 "function main():int { return 15 & 2; }"
assert_exit_code 27 "function main():int { return 26 | 3; }"
assert_exit_code 21 "function main():int { return 16 ^ 5; }"
assert_exit_code 40 "function main():int { return 5 << 3; }"
assert_exit_code 3 "function main():int { return 214 >> 6; }"
echo " Done"

echo -n "Relative: "
assert_exit_code 1 "function main():int { return 15 > 2; }"
assert_exit_code 0 "function main():int { return 16 < 6; }"
assert_exit_code 1 "function main():int { return 15 >= 15; }"
assert_exit_code 0 "function main():int { return 16 <= 2; } "
assert_exit_code 1 "function main():int { return 25 == 25; }"
assert_exit_code 0 "function main():int { return 6 != 6; }"
echo " Done"

echo -n "Local Variables: "
assert_exit_code 5 "function main():int { var a:int = 5; return a; }"
assert_exit_code 16 "function main():int { var a:int = 6; var b:int = 10; return a + b; }"
assert_exit_code 7 "function main():int { var a: int; var b:int = 7; return b; }"
assert_exit_code 12 "function main():int { var a: int; a = 12; return a; }"
assert_exit_code 24 "function main():int { var a:int = 6; var b:int = 10; a = b = 12; return a + b; }"
echo " Done"

echo -n "Functions: "
assert_exit_code 12 "function x():int { return 12; } function main():int { return x(); }"
assert_exit_code 11 "function x(y: int, z: int): int { return y + z; } function main(): int { return x(5, 6); }"
assert_compilation_error "function x():int {} function main():int { return x(1); }"
assert_compilation_error "function x(){} function main():int { return x(); }"
echo " Done"

echo -n "Typing: "
assert_compilation_error "function x() {} function main():int { return 12 ^ x(); }"
assert_compilation_error "function main(): int { var x; return 0; }"
assert_exit_code 10 "function main(): int { var x: int; x = 10; return x; }"
assert_exit_code 0 "
function main(): int {
	var i = 10;
	var ip = &i;

	var it: int = i;
	var ipt: int* = ip;

	return 0;
}
"
echo " Done"

echo -n "Scoping: "
assert_exit_code 150 "
function main(): int {
	var x: int = 5;

	{
		var y: int = 6;

		{
			var x: int = 10;
			y = x;
		}

		var z: int = 15;
		var a: int = 16;

		return y * z;
	}

	return 0;
}
"

assert_compilation_error "
function main(): int {
	var x: int = 5;

	{
		var y: int;

		y = 56;

		var y: int;
	}

	return 0;
}
"

assert_exit_code 42 "
function x(a: int): int {
	var a: int = 42;
	return a;
}

function main(): int {
	return x(6);
}
"

echo " Done"

echo -n "Print: "
assert_stdout "42" "
function main(): int {
	printu(42);
	
	return 0;
}
"

assert_stdout "5
6" "
function main(): int {
	printu(5);
	printu(6);
	
	return 0;
}
"

assert_stdout "0" "
function main(): int {
	printu(0);
	return 0;
}
"

echo " Done"

echo -n "Pointers: "

assert_compilation_error "
function main(): int {
	var ip: int* = &10;
	return 0;
}
"

assert_compilation_error "
function main(): int {
	var i: int = 10;
	var ip: int* = i;
	return 0;
}
"

assert_compilation_error "
function main(): int {
	var i: int = 10;
	var ip: int** = &(&i);
	return 0;
}
"

assert_compilation_error "
function main(): int {
	var i: int = 10;
	var ip: int* = &i;
	var ipp: int** = ip;
	return 0;
}
"

assert_compilation_error "
function main(): int {
	var i: int = *10;
	return 0;
}
"

assert_compilation_error "
function main(): int {
	var i: int = 10;
	var ip: int* = &i;
	var j: int = **ip;
	return 0;
}
"

assert_stdout "10" "
function main(): int {
	var i: int = 10;
	var ip: int* = &i;
	var j: int = *ip;
	printu(j);
	return 0;
}
"

echo " Done"