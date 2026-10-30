#!/usr/bin/env bash
JTDX_FM_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
export PATH="$JTDX_FM_ROOT/.deps/root/usr/bin:$PATH"
export LD_LIBRARY_PATH="$JTDX_FM_ROOT/.deps/root/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="$JTDX_FM_ROOT/.deps/root/usr/lib/qt/plugins"
export CMAKE_PREFIX_PATH="$JTDX_FM_ROOT/.deps/root/usr${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"
export PKG_CONFIG_PATH="$JTDX_FM_ROOT/.deps/root/usr/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
