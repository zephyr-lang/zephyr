#!/usr/bin/env bash

TEST_SUCCESS_COUNT=0
TEST_TOTAL_COUNT=0

echo "--- core.sh ---"
. tests/core.sh
echo "--- conditional.sh ---"
. tests/conditional.sh
echo "--- array.sh ---"
. tests/array.sh

echo -e "\u001b[33mSuite Complete:\u001b[0m"
echo -e "\u001b[32mSuccess:" $TEST_SUCCESS_COUNT "of" $TEST_TOTAL_COUNT "\u001b[0m"