"""Repack cepheus Android boot v0 with an edited appended DTB; never flashes."""
import argparse,hashlib,struct,zlib
from pathlib import Path

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--base-boot',type=Path,required=True)
p.add_argument('--dtb',type=Path,required=True)
p.add_argument('--out',type=Path,required=True)
a=p.parse_args()
if a.out.resolve() in (a.base_boot.resolve(),a.dtb.resolve()):
    p.error('Output must be a new file')
b=a.base_boot.read_bytes(); dtb=a.dtb.read_bytes()
assert b[:8]==b'ANDROID!', 'Not Android boot image'
ks,rs,ss,page,version=(struct.unpack_from('<I',b,o)[0] for o in (8,16,24,36,40))
assert page==4096 and version==0 and ss==0,'Only this cepheus v0/page4096/no-second format is supported'
assert dtb[:4]==bytes.fromhex('d00dfeed') and struct.unpack_from('>I',dtb,4)[0]==len(dtb),'Invalid/trailing DTB data'
align=lambda n:(n+page-1)//page*page
kernel=b[page:page+ks]
gz=zlib.decompressobj(31); raw=gz.decompress(kernel)
assert gz.eof and raw and gz.unused_data[:4]==bytes.fromhex('d00dfeed'),'Expected gzip Image followed by DTB'
old_dtb=gz.unused_data
assert struct.unpack_from('>I',old_dtb,4)[0]==len(old_dtb),'Unexpected appended data'
compressed=kernel[:-len(old_dtb)]
rd_start=page+align(ks);rd=b[rd_start:rd_start+rs]
assert len(rd)==rs
new_kernel=compressed+dtb
hdr=bytearray(b[:page]);struct.pack_into('<I',hdr,8,len(new_kernel))
digest=hashlib.sha1(new_kernel+struct.pack('<I',len(new_kernel))+rd+struct.pack('<I',len(rd))+struct.pack('<I',0)).digest()
hdr[576:608]=digest+bytes(12)
assert all(x==y or 8<=i<12 or 576<=i<608 for i,(x,y) in enumerate(zip(hdr,b[:page])))
pad=lambda v:v+bytes((-len(v))%page)
out=bytes(hdr)+pad(new_kernel)+pad(rd)
assert len(out)<=134217728,'Image exceeds this phone boot partition capacity'
assert zlib.decompress(new_kernel,31)==raw
assert out[page+align(len(new_kernel)):page+align(len(new_kernel))+rs]==rd
a.out.parent.mkdir(parents=True,exist_ok=True);a.out.write_bytes(out)
print('PASS: original compressed kernel, ramdisk, addresses and cmdline preserved')
print('bytes',len(out),'sha256',hashlib.sha256(out).hexdigest())
