#!/bin/bash
set -e
build_dir="cmake-build-debug"
target="hyriseTest"

if [ $1 = "coverage" ]; then
    target="hyriseCoverage"
fi

test_command=$target
cd $build_dir
make $target -j4
cd ".."

if [ "$target" = "hyriseTest" ]; then
    ./$build_dir/$test_command
elif [ "$target" = "hyriseCoverage" ]; then
    ./scripts/coverage.sh "$build_dir"
fi