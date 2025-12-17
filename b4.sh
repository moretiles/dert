#!/usr/bin/env bash

set -eou pipefail

make test
make test_asan
make test_tsan

# required
rm -f .b4.lock
