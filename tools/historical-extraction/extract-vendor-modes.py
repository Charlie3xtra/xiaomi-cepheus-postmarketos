from pathlib import Path
import struct,json
root=Path(__file__).resolve().parent
dest=root/'extra-sensors'
for model in ['semco_imx481','semco_s5k3m5','sunny_s5k3t1']:
    tags=json.loads((root/f'com.qti.sensormodule.{model}.tags.json').read_text())
    data=(root/'vendor'/f'com.qti.sensormodule.{model}.bin').read_bytes()
    base=max(t['pos'] for t in tags)+72
    td={t['id']:t for t in tags}
    def vals(t): return struct.unpack_from('<'+'Q'*(t['size']//8),data,base+t['offset'])
    def records(t):
        assert t['size']%80==0
        result=[]
        for off in range(0,t['size'],80):
            q=struct.unpack_from('<10Q',data,base+t['offset']+off)
            assert q[3]==1, q
            v=vals(td[q[4]])
            delays=vals(td[q[9]])
            assert len(v)==1 and len(delays)==1
            result.append(dict(addr=q[2],value=v[0],width=q[6],delay_us=delays[0],operation=q[7]))
        return result
    init=next(t for t in tags if t['name']=='initSettings')
    initq=vals(init)
    first=next(t for t in tags if t['name']=='regSetting')
    name=model.split('_',1)[1]
    initregs=records(td[initq[2]])
    mode=records(first)
    res=next(t for t in tags if t['name']=='resolutionData')
    q=vals(res)
    print(name,'init',len(initregs),'mode',len(mode),'resolution',q[:34])
    print('MODE',[(hex(r['addr']),hex(r['value']),r['width'],r['delay_us'],r['operation']) for r in mode[:7]],'LAST',mode[-4:])
    print('INIT first',initregs[:4],'nonzero delays',[(r['addr'],r['delay_us']) for r in initregs+mode if r['delay_us']])
    c=['/* Recovered from the device vendor sensor module. */\n']
    for label,regs in [('init',initregs),('mode',mode)]:
        c.append(f'static const struct aux_reg {name}_{label}[] = {{\n')
        for r in regs:
            assert r['operation']==0, r
            assert r['width'] in (1,2),r
            c.append(f'\t{{ 0x{r["addr"]:04x}, 0x{r["value"]:04x}, {r["width"]}, {r["delay_us"]} }},\n')
        c.append('};\n')
    (dest/(name+'-vendor.h')).write_text(''.join(c),newline='\n')
    (dest/(name+'-vendor.json')).write_text(json.dumps(dict(model=model,base=base,resolution=q[:34],init=initregs,mode=mode),indent=2))
