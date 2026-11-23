import os,sys,subprocess
pid=subprocess.check_output(['pgrep','-u','user','-x','xfwm4'],text=True).splitlines()[0]
env=dict(x.split('=',1) for x in open('/proc/'+pid+'/environ','rb').read().decode().split('\0') if '=' in x)
os.execvpe(sys.argv[1],sys.argv[1:],env)
