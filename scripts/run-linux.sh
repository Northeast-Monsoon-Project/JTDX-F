#!/usr/bin/env bash
set -e
FM_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
source "$FM_ROOT/scripts/env.sh"
exec "$FM_ROOT/install/bin/jtdx-f" "$@"
