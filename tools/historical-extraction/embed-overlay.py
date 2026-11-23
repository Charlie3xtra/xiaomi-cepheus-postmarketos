from pathlib import Path
r=Path(__file__).resolve().parent/'extra-sensors'
b=(r/'cepheus-aux.dtbo').read_bytes()
(r/'overlay_blob.h').write_text('static const unsigned char overlay_blob[] __aligned(8) = {\n'+','.join(str(x) for x in b)+'\n};\n',newline='\n')
