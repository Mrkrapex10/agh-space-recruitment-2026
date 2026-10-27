#!/bin/sh
# Builds and runs the host unit tests (needs gcc). Run from the project root:  sh tests/run_tests.sh
set -e
gcc -std=gnu11 -Wall -Wextra -Wpedantic -Wno-pedantic -g -fsanitize=address,undefined \
    -Ilib/tlv -IApp/Inc -Itests/stubs \
    tests/test_host.c lib/tlv/tlv.c App/Src/msg_handler.c App/Src/led_effects.c \
    -o /tmp/tlv_tests
/tmp/tlv_tests
