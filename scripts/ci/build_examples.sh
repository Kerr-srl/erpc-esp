#!/usr/bin/env bash

# Exit on error
set -e

SCRIPT_DIR=$(realpath "$(dirname "$0")")
PROJECT_ROOT_DIR="${SCRIPT_DIR}/../../"
BUILD_ROOT_DIR="${PROJECT_ROOT_DIR}/build/ci/"

echo "Sourcing ESP-IDF environment..."
. ~/esp/esp-idf/export.sh

echo -e "\nBuilding hello world example..."
idf.py -C "${PROJECT_ROOT_DIR}/src/examples/hello_world" \
    -B "${BUILD_ROOT_DIR}/examples/hello_world/host" \
    -DHOST=TRUE \
    build
idf.py -C "${PROJECT_ROOT_DIR}/src/examples/hello_world" \
    -B "${BUILD_ROOT_DIR}/examples/hello_world/target" \
    -DHOST=FALSE \
    build

echo -e "\nBuilding connection example..."
idf.py -C "${PROJECT_ROOT_DIR}/src/examples/connection" \
    -B "${BUILD_ROOT_DIR}/examples/connection/host" \
    -DHOST=TRUE \
    build
idf.py -C "${PROJECT_ROOT_DIR}/src/examples/connection" \
    -B "${BUILD_ROOT_DIR}/examples/connection/target" \
    -DHOST=FALSE \
    build

echo -e "\nBuilding esp_log example..."
idf.py -C "${PROJECT_ROOT_DIR}/src/examples/esp_log" \
    -B "${BUILD_ROOT_DIR}/examples/esp_log" \
    build

echo -e "\nOk"
