from pathlib import Path
import struct, hashlib, zlib, json, sys

r=Path(__file__).resolve().parent/'evidence'/'boot'
def sha(b): return hashlib.sha256(b).hexdigest()
def tree(b):
    header=struct.unpack_from('>10I',b)
    assert header[0]==0xd00dfeed
    off,strings=header[2],header[3]
    path=[]; result={}
    while True:
        token=struct.unpack_from('>I',b,off)[0];off+=4
        if token==1:
            end=b.index(0,off);name=b[off:end].decode();off=(end+4)&~3
            path.append(name); result['/'.join(path) or '/']={}
        elif token==2: path.pop()
        elif token==3:
            n,so=struct.unpack_from('>II',b,off);off+=8
            end=b.index(0,strings+so);name=b[strings+so:end].decode()
            result['/'.join(path) or '/'][name]=b[off:off+n];off=(off+n+3)&~3
        elif token==4: pass
        elif token==9: break
        else: raise ValueError(token)
    rm=header[4];end=rm
    while b[end:end+16]!=bytes(16):end+=16
    return result,b[rm:end+16]

nominal = '--nominal' in sys.argv
old=(r/'baseline.dtb').read_bytes();dtb=(r/('ssc-pool-nominal.dtb' if nominal else 'ssc-pool.dtb')).read_bytes()
assert sha(old)=='b87670e8dc88f2720acd6f0aabda8ab408a617649009868df1051039e527018b'
a,ar=tree(old);b,br=tree(dtb);assert ar==br
pool='/reserved-memory/cepheus-ssc-pool'
fpath='/soc@0/remoteproc@2400000/glink-edge/fastrpc'
allowed={(fpath,'memory-region'),(fpath,'qcom,vmids'),('/__symbols__','cepheus_ssc_pool')}
if nominal:
    allowed.add(('/soc@0/clock-controller@ad00000','required-opps'))
    old_req = a['/soc@0/clock-controller@ad00000']['required-opps']
    old_opp = next(p for p,v in a.items() if v.get('phandle') == old_req)
    old_table = old_opp.rsplit('/',1)[0]
    nominal_nodes = [p for p,v in b.items() if p.rsplit('/',1)[0] == old_table and v.get('opp-level') == struct.pack('>I',256)]
    assert len(nominal_nodes) == 1
    for p in nominal_nodes:
        allowed.add((p,'phandle'))
    cam_req=b['/soc@0/clock-controller@ad00000']['required-opps']
    assert any(b[p].get('phandle') == cam_req for p in nominal_nodes)
changes=[]
for path in sorted(a.keys()|b.keys()):
    assert path in b, ('removed node',path)
    if path not in a:
        assert path in (pool,'/__symbols__'),('unexpected node',path)
    for prop in sorted(a.get(path,{}).keys()|b[path].keys()):
        before=a.get(path,{}).get(prop);after=b[path].get(prop)
        if before!=after:
            assert path==pool or (path,prop) in allowed,(path,prop)
            changes.append(path+'/'+prop)
assert b[pool]['size']==struct.pack('>II',0,0x1000000)
assert b[pool]['alignment']==struct.pack('>II',0,0x400000)
assert b[pool]['alloc-ranges']==struct.pack('>4I',0,0,0,0xffffffff)
assert b[pool]['compatible']==b'shared-dma-pool\0'
assert b[pool]['reusable']==b''
assert b[fpath]['qcom,vmids']==struct.pack('>3I',3,5,6)
assert b[fpath]['memory-region']==b[pool]['phandle']
assert b['/soc@0/remoteproc@2400000']['status']==b'disabled\0'

baseline=(r/'boot-baseline.img').read_bytes()
assert sha(baseline)=='0fc704c54c99ed275b84974b89f677c38045967bb5c23ffb258320d70a38e814'
hdr=bytearray(baseline[:4096]);gz=(r/'Image-clean.gz').read_bytes()
assert sha(zlib.decompress(gz,31))=='b7b92e2053c3c782d70a5ce7f1848015c826a62b3db6ff42beae102577844421'
rd=(r/'baseline-ramdisk.cpio.gz').read_bytes();kernel=gz+dtb
struct.pack_into('<I',hdr,8,len(kernel))
digest=hashlib.sha1(kernel+struct.pack('<I',len(kernel))+rd+struct.pack('<I',len(rd))+struct.pack('<I',0)).digest()
hdr[576:608]=digest+bytes(12)
assert all(x==y or 8<=i<12 or 576<=i<608 for i,(x,y) in enumerate(zip(hdr,baseline[:4096])))
def pad(v):return v+bytes((-len(v))%4096)
img=bytes(hdr)+pad(kernel)+pad(rd)
assert len(img)<len(baseline)
z=zlib.decompressobj(31);assert z.decompress(kernel)==zlib.decompress(gz,31);assert z.unused_data==dtb
rd_start=4096+len(pad(kernel));assert img[rd_start:rd_start+len(rd)]==rd
(r/('boot-gpu-ssc-nominal.img' if nominal else 'boot-gpu-ssc-pool.img')).write_bytes(img)
report=dict(status='PASS',dtb_sha256=sha(dtb),image_sha256=sha(img),image_bytes=len(img),
            kernel_identical_to_verified_gpu_kernel=True,ramdisk_identical=True,
            original_dtb_properties_preserved=not nominal,
            unrelated_original_dtb_properties_preserved=True,
            slpi_kept_disabled=True,changes=changes)
report['camera_nominal_required_opp'] = nominal
(r/('ssc-nominal-verification.json' if nominal else 'ssc-pool-verification.json')).write_text(json.dumps(report,indent=2),newline='\n')
print(json.dumps(report,indent=2))
