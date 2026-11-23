from pathlib import Path
import sys
if len(sys.argv)!=3:raise SystemExit('usage: python embed-overlay.py input.dtbo output.h')
b=Path(sys.argv[1]).read_bytes()
assert b[:4]==bytes.fromhex('d00dfeed')
Path(sys.argv[2]).write_text('static const unsigned char overlay_blob[] __aligned(8) = {\n'+','.join(map(str,b))+'\n};\n',newline='\n')
