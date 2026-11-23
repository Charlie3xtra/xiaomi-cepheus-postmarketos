# Experimental telephoto one-shot autofocus

Source: corrected 2026-11-07 prototype; C++ despite the earlier .c filename. It uses libcamera XRGB8888 software-ISP frames and direct I2C to the verified telephoto AK7374 (bus 4, 7-bit address 0x0c). Motor writes are restricted to that device and DAC range 0..1023. It does not implement main-camera AF or application-integrated continuous AF.

Compile against the matching custom libcamera development headers/libraries:

```sh
PKG_CONFIG_PATH=/opt/cepheus-camera/lib/pkgconfig \
  c++ -std=c++17 -O2 -pthread cepheus-tele-af.cpp \
  $(PKG_CONFIG_PATH=/opt/cepheus-camera/lib/pkgconfig pkg-config --cflags --libs libcamera) \
  -o cepheus-tele-af-20261107
```

The archived wrapper expects that binary under /usr/local/libexec/, the matching cepheus-camera-session wrapper, and cepheus-af-rail.service. The latter is archived under userspace/installed-config/20261123; the hardware-specific GPIO mapping must be audited before reuse. No automatic installer or default invocation of the inherited main-camera defaults is provided.

After closing other camera clients, the already-installed device command was sudo cepheus-tele-af. The wrapper supplies --bus=4 --addr=0x0c --max=1023 --chip=ak7374 --camera=tele. The final position remains active/held; this is a finite sweep, not a lens-position calibration valid for all scenes. A failed sweep or timeout does not prove the motor moved to a useful focus.
