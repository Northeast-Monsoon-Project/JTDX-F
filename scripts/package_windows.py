#!/usr/bin/env python3
"""Collect the Windows runtime with an audited, recursive PE import closure."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import shutil
import subprocess

root=Path(__file__).resolve().parent.parent
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--prefix',default=str(root/'install-windows'))
args=parser.parse_args()
prefix=Path(args.prefix); bindir=prefix/'bin'; bindir.mkdir(parents=True,exist_ok=True)
sdk=root/'.deps/msys2/ucrt64'
compiler=root/'.deps/root/usr/x86_64-w64-mingw32/bin'
objdump=root/'.deps/root/usr/bin/x86_64-w64-mingw32-objdump'
lookup={}
for directory in (sdk/'bin',compiler):
    for p in directory.glob('*.dll'):lookup[p.name.lower()]=p
system=set('dwrite advapi32 bcrypt bcryptprimitives cfgmgr32 comctl32 comdlg32 crypt32 d3d9 d3d11 d3dcompiler_47 ddraw dnsapi dwmapi dxgi dxguid dxva2 gdi32 imm32 iphlpapi kernel32 mpr msvcrt ncrypt netapi32 ntdll ole32 oleaut32 opengl32 powrprof propsys psapi rpcrt4 secur32 setupapi shell32 shlwapi user32 userenv usp10 uxtheme version winhttp wininet winmm winspool wldap32 ws2_32 wtsapi32 normaliz ucrtbase windowscodecs msimg32 wintrust hid authz avrt mf mfplat mfreadwrite mfuuid'.split())
for kind,names in {'platforms':['qwindows.dll'],'audio':['qtaudio_windows.dll'],
                   'imageformats':['qjpeg.dll','qico.dll','qgif.dll']}.items():
    dest=prefix/'plugins'/kind;dest.mkdir(parents=True,exist_ok=True)
    for name in names:shutil.copy2(sdk/'share/qt5/plugins'/kind/name,dest/name)
# Qt loads OpenSSL at runtime instead of listing it in the PE import table.
for glob in ('libssl-*.dll','libcrypto-*.dll'):
    for p in (sdk/'bin').glob(glob):shutil.copy2(p,bindir/p.name)
queue=list(bindir.glob('*.exe'))+list(bindir.glob('*.dll'))+list((prefix/'plugins').rglob('*.dll'))
seen=set();missing=set();manifest={}
while queue:
    p=queue.pop()
    if str(p) in seen:continue
    seen.add(str(p))
    output=subprocess.check_output([str(objdump),'-p',str(p)],text=True,stderr=subprocess.STDOUT)
    imports=re.findall(r'DLL Name: (\S+)',output)
    for name in imports:
        key=name.lower()
        if key.startswith(('api-ms-','ext-ms-')) or key.removesuffix('.dll') in system:continue
        source=lookup.get(key)
        if source is None:missing.add(name);continue
        dest=bindir/source.name
        if source!=dest:shutil.copy2(source,dest)
        queue.append(dest)
    manifest[str(p.relative_to(prefix))]={'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'imports':imports}
if missing:raise SystemExit('Unresolved Windows imports: '+', '.join(sorted(missing)))
(bindir/'qt.conf').write_text('[Paths]\nPlugins = ../plugins\nTranslations = ../translations\n')
(prefix/'runtime-manifest.json').write_text(json.dumps(manifest,indent=2))
for name in ('COPYING','AUTHORS','THANKS'):
    shutil.copy2(root/'jtdx-master'/name,prefix/name)
shutil.copytree(root/'jtdx-master/docs',prefix/'share/jtdx/docs',dirs_exist_ok=True)
shutil.copytree(root/'jtdx-master/tools',prefix/'share/jtdx/tools',dirs_exist_ok=True)
print(f'Bundled {len(manifest)} PE files; every non-system import resolved')

shutil.copytree(sdk/'share/licenses',prefix/'licenses/msys2',dirs_exist_ok=True)
for package,source in [('hamlib',root/'.deps/hamlib-4.7.2'),('libusb',root/'.deps/libusb-1.0.30')]:
    dest=prefix/'licenses'/package;dest.mkdir(parents=True,exist_ok=True)
    for p in source.glob('COPYING*'):shutil.copy2(p,dest/p.name)
dest=prefix/'licenses/gcc';dest.mkdir(parents=True,exist_ok=True)
for name in ('COPYING3','COPYING.RUNTIME'):
    shutil.copy2(sdk/'share/licenses/libgcc'/name,dest/name)
for package in ('mingw-w64-winpthreads','mingw-w64-headers','mingw-w64-crt'):
    shutil.copytree(root/'.deps/root/usr/share/licenses'/package,prefix/'licenses'/package,dirs_exist_ok=True)
shutil.copy2(root/'.deps/msys2/manifest-ucrt64.json',prefix/'manifest-ucrt64.json')

# User QSO data and private QA fixtures must never be distributed.
for p in prefix.rglob("*"):
    if p.is_file() and (p.suffix.lower() in (".adi", ".adif", ".log") or p.name == "wsjtx_log.adi"):
        raise SystemExit("Log file found in runtime payload: "+str(p))
