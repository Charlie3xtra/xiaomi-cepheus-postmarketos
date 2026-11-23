import pathlib, json, hashlib, sys
import numpy as np
from PIL import Image
src = pathlib.Path(sys.argv[1])
out = pathlib.Path(sys.argv[2])
raw = src.read_bytes()
width, height, stride = (map(int,sys.argv[3:6]) if len(sys.argv)>=6 else (4000,3000,5008))
pattern = sys.argv[6] if len(sys.argv)>6 else 'RGGB'
assert len(raw) == stride * height, len(raw)
a = np.frombuffer(raw, np.uint8).reshape(height, stride)[:, :width*5//4]
b = a.reshape(height, width//4, 5).astype(np.uint16)
p = np.empty((height, width), np.uint16)
for i in range(4):
    p[:, i::4] = (b[:, :, i] << 2) | ((b[:, :, 4] >> (2*i)) & 3)
stats = dict(bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest(),
             nonzero_bytes=int(np.count_nonzero(np.frombuffer(raw,np.uint8))),
             minimum=int(p.min()), maximum=int(p.max()), mean=float(p.mean()),
             percentiles=np.percentile(p,[0,1,10,50,90,99,100]).tolist(),
             row_mean_range=[float(p.mean(1).min()),float(p.mean(1).max())])
print(json.dumps(stats,indent=2))
out.with_suffix('.json').write_text(json.dumps(stats,indent=2),encoding='utf-8')
if pattern=='GRBG':
    rgb = np.stack([p[0::2,1::2],(p[0::2,0::2].astype(np.float32)+p[1::2,1::2])/2,p[1::2,0::2]],axis=2).astype(np.float32)
else:
    rgb = np.stack([p[0::2,0::2],(p[0::2,1::2].astype(np.float32)+p[1::2,0::2])/2,p[1::2,1::2]],axis=2).astype(np.float32)
rgb = np.maximum(rgb-64,0)
means = rgb.mean(axis=(0,1))
rgb *= (means.mean()/np.maximum(means,1))[None,None,:]
white = max(float(np.percentile(rgb,99.5)),1)
rgb = np.clip(rgb/white,0,1)**(1/2.2)
Image.fromarray((rgb*255).astype(np.uint8)).resize((1000,round(height/width*1000))).save(out)
