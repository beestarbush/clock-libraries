#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"

if [[ "${IN_QTBUILDER:-0}" != "1" ]]; then
    docker run --rm -i \
        --user "$(id -u):$(id -g)" \
        -e IN_QTBUILDER=1 \
        -v "${SCRIPT_DIR}:/workdir" \
        qtbuilder ./build.sh "${@:-}"
    exit 0
fi

CLEAN="${1:-}"
BUILD_DIR="build"
GENERATOR="Ninja"

if [[ "$CLEAN" == "--clean" ]]; then
    echo "Cleaning build directory..."
    rm -rf "${BUILD_DIR}"
fi

mkdir -p "${BUILD_DIR}"

CACHE_FILE="${BUILD_DIR}/CMakeCache.txt"
if [[ -f "${CACHE_FILE}" ]]; then
    CACHED_GENERATOR=$(sed -n 's/^CMAKE_GENERATOR:INTERNAL=//p' "${CACHE_FILE}")
    if [[ "${CACHED_GENERATOR}" != "${GENERATOR}" ]]; then
        echo "Reconfiguring build directory for current generator..."
        rm -rf "${BUILD_DIR}/CMakeCache.txt" "${BUILD_DIR}/CMakeFiles"
    fi
fi

cmake -S . -B "${BUILD_DIR}" -G "${GENERATOR}"
cmake --build "${BUILD_DIR}"
