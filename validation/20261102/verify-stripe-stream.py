from pathlib import Path
import json,sys,time
reference=Path('/home/user/stripe-reference.raw').read_bytes()
count=bad=0;start=time.monotonic();first=[]
with open(sys.argv[1],'rb',buffering=0) as f:
 while True:
  a=bytearray()
  while len(a)<len(reference):
   b=f.read(len(reference)-len(a))
   if not b:break
   a.extend(b)
  if not a:break
  if len(a)!=len(reference):print('SHORT',len(a),flush=True);bad+=1;break
  count+=1
  if a!=reference:
   wrong=sum(a[y*5008:y*5008+5000]!=reference[y*5008:y*5008+5000] for y in range(3000))
   if wrong:
    bad+=1
    if len(first)<5:first.append(dict(frame=count,wrong_rows=wrong))
  if count%300==0:print(json.dumps(dict(frames=count,bad_frames=bad,seconds=round(time.monotonic()-start,2))),flush=True)
print(json.dumps(dict(frames=count,bad_frames=bad,first_bad=first,seconds=round(time.monotonic()-start,2))),flush=True)
sys.exit(bool(bad) or count==0)
