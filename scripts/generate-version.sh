#!/bin/bash

function main() {
    local project_dir
    project_dir=$(git rev-parse --show-toplevel)

    git submodule update
    cd "$project_dir/opentracing-cpp" || exit 1
    rm -rf build
    mkdir build
    cd build
    cmake ..
    # No need to actually build project, just generating version.h
}

main
