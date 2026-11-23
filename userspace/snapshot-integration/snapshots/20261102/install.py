from pathlib import Path
import shutil,os,platform,datetime
assert platform.machine()=="aarch64"
assert Path("/opt/cepheus-camera/lib/libcamera.so.0.7.2").exists()
r=Path('/home/user/system-camera-integration');backup=Path('/var/backups/cepheus-system-camera-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S'));backup.mkdir(exist_ok=True)
def put(dst,data,mode=0o644,user=False):
 p=Path(dst)
 if p.exists():
  b=backup/p.relative_to('/');b.parent.mkdir(parents=True,exist_ok=True)
  if not b.exists():shutil.copy2(p,b)
 p.parent.mkdir(parents=True,exist_ok=True);p.write_text(data,encoding='utf-8',newline='\n');p.chmod(mode)
 if user:os.chown(p,10000,10000)
put('/etc/cepheus-camera.env',(r/'camera.env').read_text())
for name in ['cepheus-camera-session','cepheus-snapshot','cepheus-pipewire-launcher']:
 put('/usr/local/libexec/'+name,(r/name).read_text(),0o755)
for unit in ['pipewire','wireplumber','serverdeck-desktop']:
 put('/home/user/.config/systemd/user/'+unit+'.service.d/50-cepheus-camera.conf','[Service]\nEnvironmentFile=/etc/cepheus-camera.env\n',user=True)
s=Path('/usr/share/applications/org.gnome.Snapshot.desktop').read_text().replace('Exec=snapshot','Exec=/usr/local/libexec/cepheus-snapshot').replace('DBusActivatable=true','DBusActivatable=false')
put('/home/user/.local/share/applications/org.gnome.Snapshot.desktop',s,user=True)
put('/home/user/.local/share/dbus-1/services/org.gnome.Snapshot.service','[D-BUS Service]\nName=org.gnome.Snapshot\nExec=/usr/local/libexec/cepheus-snapshot --gapplication-service\n',user=True)
s=Path('/etc/xdg/autostart/pipewire.desktop').read_text().replace('Exec=/usr/libexec/pipewire-launcher','Exec=/usr/local/libexec/cepheus-pipewire-launcher')
for cfg in ['.config','.config/serverdeck-desktop']:
 put('/home/user/'+cfg+'/autostart/pipewire.desktop',s,user=True)
# New directories must belong to the user as well.
for relative in ['.config/systemd/user/pipewire.service.d','.config/systemd/user/wireplumber.service.d','.config/systemd/user/serverdeck-desktop.service.d','.local/share/dbus-1','.local/share/dbus-1/services','.config/serverdeck-desktop/autostart']:
 os.chown('/home/user/'+relative,10000,10000)
print('Installed scoped CPU ISP integration for Snapshot, PipeWire and ServerDeck')

shutil.copy2(r/'libcepheus-gst-copy.so','/opt/cepheus-camera/lib/libcepheus-gst-copy.so')
os.chmod('/opt/cepheus-camera/lib/libcepheus-gst-copy.so',0o644)
for cfg in ['.config','.config/serverdeck-desktop']:
 put('/home/user/'+cfg+'/wireplumber/wireplumber.conf.d/51-cepheus-cameras.conf',(r/'51-cepheus-cameras.conf').read_text(),user=True)
 os.chown('/home/user/'+cfg+'/wireplumber',10000,10000)
 os.chown('/home/user/'+cfg+'/wireplumber/wireplumber.conf.d',10000,10000)
