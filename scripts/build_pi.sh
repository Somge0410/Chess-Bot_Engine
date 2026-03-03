#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "=== Building Chess-Bot_Engine (Release) ==="
cmake -S "$PROJECT_DIR" -B "$PROJECT_DIR/build" \
      -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DUSE_NATIVE_ARCH=ON

cmake --build "$PROJECT_DIR/build" -j"$(nproc)"

echo "=== Build complete: $PROJECT_DIR/build/Chess-Bot_Engine ==="