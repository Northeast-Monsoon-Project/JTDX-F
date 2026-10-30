#!/usr/bin/env bash
set -euo pipefail
FM_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$FM_ROOT"
source scripts/env.sh
cmake -S jtdx-master -B build-windows-ucrt -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$FM_ROOT/scripts/windows-toolchain.cmake" \
  -DCMAKE_BUILD_TYPE=Release -DWSJT_GENERATE_DOCS=OFF -DWSJT_SKIP_MANPAGES=ON \
  -DCMAKE_INSTALL_PREFIX="$FM_ROOT/install-windows"
cmake --build build-windows-ucrt -j "${FM_JOBS:-6}"
cmake --install build-windows-ucrt
python3 scripts/package_windows.py
