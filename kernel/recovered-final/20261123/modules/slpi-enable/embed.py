from pathlib import Path
p = Path(__file__).resolve().parent
b = (p/'cepheus-slpi.dtbo').read_bytes()
(p/'overlay_blob.h').write_text('static const unsigned char overlay_blob[] = {\n'+
    ','.join(hex(v) for v in b)+'\n};\n', newline='\n')
