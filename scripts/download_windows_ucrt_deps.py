from pathlib import Path
import json,re,subprocess,concurrent.futures,hashlib
root=Path(__file__).resolve().parent.parent
packages=root/'.deps/msys2';packages.mkdir(parents=True,exist_ok=True)
locked=root/'dependencies/manifest-ucrt64.json'
db=json.loads((locked if locked.exists() else packages/'db-ucrt64.json').read_text())
prefix='mingw-w64-ucrt-x86_64-'
selected=set()
def add(n):
 if n in selected: return
 if n not in db:
  alt=next((k for k,v in db.items() if n in [re.split('[<>=]',x)[0] for x in v.get('PROVIDES',[])]),None)
  if not alt: raise RuntimeError('Missing '+n)
  n=alt
 selected.add(n)
 for d in db[n].get('DEPENDS',[]): add(re.split('[<>=]',d)[0])
for n in ['qt5-base','qt5-multimedia','qt5-serialport','qt5-websockets','qt5-tools','qt5-activeqt','fftw','boost','nsis']: add(prefix+n)
(packages/'manifest-ucrt64.json').write_text(json.dumps({n:db[n] for n in sorted(selected)},indent=2))
print(len(selected),'packages',sum(int(db[n]['CSIZE'][0]) for n in selected)//1024//1024,'MiB',flush=True)
def get(n):
 d=db[n];f=packages/d['FILENAME'][0]
 if not f.exists() or hashlib.sha256(f.read_bytes()).hexdigest()!=d['SHA256SUM'][0]:
  subprocess.run(['curl','-fsSL','-C','-','--retry','3','--connect-timeout','20','--max-time','600','https://repo.msys2.org/mingw/ucrt64/'+f.name,'-o',str(f)],check=True)
 if hashlib.sha256(f.read_bytes()).hexdigest()!=d['SHA256SUM'][0]: raise RuntimeError('Hash mismatch '+n)
 return f
pool=concurrent.futures.ThreadPoolExecutor(max_workers=4)
for future in concurrent.futures.as_completed([pool.submit(get,n) for n in sorted(selected)]):
 f=future.result()
 subprocess.run(['bsdtar','-xf',str(f),'-C',str(packages)],check=True)
 print(f.name,flush=True)
