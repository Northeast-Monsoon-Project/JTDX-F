#!/usr/bin/env bash
set -euo pipefail
FM_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$FM_ROOT"
source scripts/env.sh
export WINEPREFIX="$FM_ROOT/.deps/wine"
export DISPLAY="${FM_WINE_DISPLAY:-:97}" WAYLAND_DISPLAY=""
export WINEDLLOVERRIDES="mscoree,mshtml=" WINEDEBUG=-all
mkdir -p release
FM_VERSION="$(python3 -c 'import re,pathlib;s=pathlib.Path("jtdx-master/Versions.cmake").read_text();print(".".join(re.search(r"set \(WSJTX_VERSION_"+k+r" (\d+)\)",s).group(1) for k in ("MAJOR","MINOR","SUB")))')"
python3 - <<'PY'
from pathlib import Path
prefix=Path('install-windows')
files=sorted(p for p in prefix.rglob('*') if p.is_file())
lines=['Delete "$INSTDIR\\'+str(p.relative_to(prefix)).replace('/', '\\')+'"' for p in files]
dirs=sorted((p for p in prefix.rglob('*') if p.is_dir()),key=lambda p:len(p.parts),reverse=True)
lines+=['RMDir "$INSTDIR\\'+str(p.relative_to(prefix)).replace('/', '\\')+'"' for p in dirs]
Path('release/uninstall-files.nsh').write_text('\n'.join(lines)+'\n')
PY
FM_WIN_ROOT="Z:${FM_ROOT//\//\\}"
export NSISDIR="$FM_WIN_ROOT\.deps\msys2\ucrt64\share\nsis"
WINEPATH="Z:$FM_ROOT/install-windows/bin;Z:$FM_ROOT/.deps/msys2/ucrt64/bin" \
  wine .deps/msys2/ucrt64/bin/makensis.exe -V2 -NOCD -INPUTCHARSET UTF8 \
  "-DFM_VERSION=$FM_VERSION" \
  "-DFM_STAGE=$FM_WIN_ROOT\install-windows" \
  "-DFM_OUTPUT=$FM_WIN_ROOT\release\JTDX-F-V${FM_VERSION}-Windows-x64-Setup.exe" \
  "-DFM_ICON=$FM_WIN_ROOT\jtdx-master\icons\windows-icons\jtdx.ico" \
  "-DFM_UNINSTALL_FILES=$FM_WIN_ROOT\release\uninstall-files.nsh" \
  "Z:$FM_ROOT/scripts/windows-installer.nsi"
