#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cmake -S "$ROOT_DIR" -B "$ROOT_DIR/build" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build "$ROOT_DIR/build" --config Debug --parallel
ctest --test-dir "$ROOT_DIR/build" --build-config Debug --output-on-failure
