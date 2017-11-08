#!/bin/bash

set -e

function main() {
    local project_dir
    project_dir=$(git rev-parse --show-toplevel)

    cd "$project_dir" || exit 1
    git submodule update --init
    cd opentracing-cpp || exit 1
    rm -rf build install
    mkdir build && cd build || exit 1
    cmake -DCMAKE_INSTALL_PREFIX="$project_dir/opentracing-cpp/install" ..
    make
    make install
}

main
