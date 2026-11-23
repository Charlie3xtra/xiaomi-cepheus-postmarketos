import struct

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
