"""Extract only camera/power DT source, excluding unrelated device identifiers."""
from pathlib import Path
import re,struct
from fdt_tree import tree
root=Path(__file__).resolve().parents[1]
text=(root.parent/'handoff-live-tree-20261106.dts').read_text()
lines=text.splitlines(True);stack=[];nodes={}
for i,line in enumerate(lines):
    m=re.match(r'^\s*([^\s{};]+)\s*\{\s*$',line)
    if m:
        name=m[1]; stack.append((name,i))
    elif re.match(r'^\s*};\s*$',line):
        path='/'+'/'.join(n for n,_ in stack if n!='/')
        _,start=stack.pop();nodes[path]=''.join(lines[start:i+1])
main='/soc@0/cci@ac4a000/i2c-bus@1/camera@1a'
wanted=[main,'/gpio-regulator-1']
base,_=tree((root.parent/'evidence/boot/ssc-pool-nominal.dtb').read_bytes())
phandles={0xc3,0xc4,0xc5,0xf0,0xf2,0xc6,0xc7,0xc8}
for path,block in nodes.items():
    # Only the direct properties of the node, not descendants.
    direct=block.split('\n\n',1)[0]
    m=re.search(r'phandle = <0x([0-9a-f]+)>;',direct)
    if m and int(m[1],16) in phandles:wanted.append(path)
out=['/* Extracted from the running phone on 2026-11-06. Reference fragments.\n',
     ' * Numeric phandles refer to this exact device tree. Not a standalone DTS.\n',
     ' * No lens-focus / VCM node currently exists.\n */\n\n']
for path in dict.fromkeys(wanted):
    assert path in nodes,path
    block=nodes[path]
    for prop in ('regulator-min-microvolt','regulator-max-microvolt'):
        raw=base.get(path,{}).get(prop)
        if raw and len(raw)==4:
            block=re.sub(r'(?m)^(\s*'+prop+r' = ).*?;',lambda m:m[1]+'<0x'+format(struct.unpack('>I',raw)[0],'x')+'>;',block)
    out+=['/* PATH: '+path+' */\n',block,'\n']
(root/'dt/live-camera-power-fragments.dtsi').write_text(''.join(out),newline='\n')
assert 'regulator-name = "actuator_rear_regulator"' in ''.join(out)
assert 'compatible = "sony,imx586"' in ''.join(out)
print('Extracted',len(dict.fromkeys(wanted)),'camera/power nodes')
