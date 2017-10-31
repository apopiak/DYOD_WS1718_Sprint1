#!/bin/bash
set -e
build_dir="cmake-build-debug"
target="hyriseTest"
test_command=$target
cd $build_dir
make $target -j4
cd ".."
./$build_dir/$test_command