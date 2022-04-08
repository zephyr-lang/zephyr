#!/usr/bin/env bash

. tests/asserts.sh

echo -n "Ternary Expression: "
assert_exit_code 2 "function main(): int { return 0 ? 10 : 2; }"
assert_exit_code 10 "function main(): int { return 1 ? 10 : 2; }"
assert_compilation_error "function x() {} function main: int { return x() ? 1 : 2; }"
echo " Done"