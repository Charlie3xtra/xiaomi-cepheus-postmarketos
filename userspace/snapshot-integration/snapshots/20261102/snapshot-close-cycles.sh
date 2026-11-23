set -eu
ui() { runuser -u user -- python3 /home/user/desktop-exec.py "$@"; }
# Close the existing preview, then exercise the real desktop menu five times.
ui xdotool mousemove 763 34 click 1
sleep 1
! pgrep -u user -x snapshot
for round in 1 2 3 4 5; do
 ui xfce4-popup-applicationsmenu
 sleep 0.3
 ui xdotool mousemove 54 653
 sleep 0.5
 ui xdotool mousemove 163 655 click 1
 sleep 4
 pid=$(pgrep -u user -x snapshot)
 echo "round=$round pid=$pid fds=$(ls /proc/$pid/fd | wc -l)"
 grep serverdeck-desktop /proc/$pid/cgroup
 ui xdotool mousemove 763 34 click 1
 sleep 1
 ! pgrep -u user -x snapshot
 echo "round=$round closed=ok"
done
cat /proc/sys/kernel/random/boot_id
dmesg | grep -E 'WARNING:|Oops:|rcu:.*stall' | tail -6
