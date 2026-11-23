from pathlib import Path
b = Path("cepheus-uart13.dtbo").read_bytes()
Path("overlay_blob.h").write_text("static const unsigned char overlay_blob[] = {\n" + ",".join(hex(x) for x in b) + "\n};\n")
