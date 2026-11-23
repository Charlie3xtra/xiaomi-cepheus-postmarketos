from pathlib import Path
import struct, subprocess
from fdt_tree import tree

r=Path(__file__).resolve().parent/'evidence'/'boot'
data=(r/'ssc-pool.dtb').read_bytes();nodes,_=tree(data)
path='/soc@0/clock-controller@ad00000'
old_phandle=nodes[path]['required-opps']
old_node=next(p for p,v in nodes.items() if v.get('phandle')==old_phandle)
parent=old_node.rsplit('/',1)[0]
target=next(p for p,v in nodes.items() if p.rsplit('/',1)[0]==parent and v.get('opp-level')==struct.pack('>I',256))
out=r/'ssc-pool-nominal.dtb';out.write_bytes(data)
wsl='/mnt/c/'+str(out).split(':',1)[1].lstrip('\\/').replace('\\','/')
def put(node,prop,value):
    subprocess.run(['wsl','-d','Debian','fdtput','-t','x',wsl,node,prop,format(value,'x')],check=True)
phandle=nodes[target].get('phandle')
if phandle is None:
    phandle=struct.pack('>I',1+max(struct.unpack('>I',v['phandle'])[0] for v in nodes.values() if 'phandle' in v))
    put(target,'phandle',struct.unpack('>I',phandle)[0])
put(path,'required-opps',struct.unpack('>I',phandle)[0])
print('Camera required OPP:',old_node,'->',target)
print('Previous level:',struct.unpack('>I',nodes[old_node]['opp-level'])[0],'new level: 256')
