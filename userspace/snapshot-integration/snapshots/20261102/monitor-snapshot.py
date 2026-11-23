import time,json,subprocess
from pathlib import Path
p=subprocess.check_output(['pgrep','-u','user','-x','snapshot'],text=True).strip()
start=time.monotonic();samples=[]
for i in range(37):
 proc=Path('/proc',p)
 if not proc.exists():raise SystemExit('Snapshot exited during stability test')
 row=dict(seconds=round(time.monotonic()-start,2),pid=int(p),fds=len(list((proc/'fd').iterdir())))
 samples.append(row);print(json.dumps(row),flush=True)
 if i<36:time.sleep(5)
print(json.dumps(dict(result='complete',seconds=round(time.monotonic()-start,2),fd_min=min(x['fds'] for x in samples),fd_max=max(x['fds'] for x in samples))),flush=True)
