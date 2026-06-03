#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"

if [[ "${IN_QTBUILDER:-0}" != "1" ]]; then
    docker run --rm -i \
        --user "$(id -u):$(id -g)" \
        -e IN_QTBUILDER=1 \
        -e QT_QPA_PLATFORM=offscreen \
        -v "${SCRIPT_DIR}:/workdir" \
        qtbuilder ./run-tests.sh
    exit 0
fi

BUILD_DIR="build"

if [[ ! -d "${BUILD_DIR}" ]]; then
    echo "Build directory not found. Running ./build.sh first..."
    ./build.sh
fi

ctest --test-dir "${BUILD_DIR}" --output-on-failure
