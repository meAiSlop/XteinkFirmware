#!/usr/bin/env bash
set -euo pipefail
export SOURCE_DATE_EPOCH=${SOURCE_DATE_EPOCH:-1704067200}
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
sha256sum build/xteink_main > build/xteink_main.sha256
