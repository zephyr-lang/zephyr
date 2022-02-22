#!/usr/bin/env bash

. tests/asserts.sh

echo -n "Basic returns: "
assert_exit_code "function main(): int { return 0; }" 0
assert_exit_code "function main(): int { return 100; }" 100
echo " Done"

echo -n "Unary: "
assert_exit_code "function main(): int { return -1; }" 255 # Unsigned
assert_exit_code "function main(): int { return !56; }" 0
assert_exit_code "function main(): int { return !0; }" 1
assert_exit_code "function main(): int { return ~2; }" 253 # Unsigned
assert_exit_code "function main(): int { return !~0; }" 0
echo " Done"

echo -n "Arithmetic: "
assert_exit_code "function main():int { return 5 + 3; }" 8
assert_exit_code "function main():int { return 12 - 9; }" 3
assert_exit_code "function main():int { return 8 * 7; }" 56
assert_exit_code "function main():int { return 37 / 3; }" 12
assert_exit_code "function main():int { return 37 % 3; }" 1
assert_exit_code "function main():int { return 5 + 3 * 5; }" 20
assert_exit_code "function main():int { return (5 + 3) * 5; }" 40
echo " Done"

echo -n "Bitwise: "
assert_exit_code "function main():int { return 15 & 2; }" 2
assert_exit_code "function main():int { return 26 | 3; }" 27
assert_exit_code "function main():int { return 16 ^ 5; }" 21
assert_exit_code "function main():int { return 5 << 3; }" 40
assert_exit_code "function main():int { return 214 >> 6; }" 3
echo " Done"

echo -n "Relative: "
assert_exit_code "function main():int { return 15 > 2; }" 1
assert_exit_code "function main():int { return 16 < 6; }" 0
assert_exit_code "function main():int { return 15 >= 15; }" 1
assert_exit_code "function main():int { return 16 <= 2; } " 0
assert_exit_code "function main():int { return 25 == 25; }" 1
assert_exit_code "function main():int { return 6 != 6; }" 0
echo " Done"

echo -n "Local Variables: "
assert_exit_code "function main():int { var a:int = 5; return a; }" 5
assert_exit_code "function main():int { var a:int = 6; var b:int = 10; return a + b; }" 16
assert_exit_code "function main():int { var a: int; var b:int = 7; return b; }" 7
assert_exit_code "function main():int { var a: int; a = 12; return a; }" 12
assert_exit_code "function main():int { var a:int = 6; var b:int = 10; a = b = 12; return a + b; }" 24
echo " Done"

echo -n "Functions: "
assert_exit_code "function x():int { return 12; } function main():int { return x(); }" 12
echo " Done"