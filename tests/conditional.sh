#!/usr/bin/env bash

. tests/asserts.sh

echo -n "Ternary Expression: "
assert_exit_code "function main(): int { return 0 ? 10 : 2; }" 2
assert_exit_code "function main(): int { return 1 ? 10 : 2; }" 10
assert_compilation_error "function x() {} function main: int { return x() ? 1 : 2; }"
echo " Done"