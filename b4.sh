#!/usr/bin/env bash

set -eou pipefail

make test
./test
make test_asan
./test
make test_tsan
./test

# required
rm -f .b4.lock
