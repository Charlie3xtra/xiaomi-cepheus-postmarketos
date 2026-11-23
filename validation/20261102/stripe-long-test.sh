set -eu
base=/home/user/stripe-validation
for d in /dev/v4l-subdev*; do n=$(cat /sys/class/video4linux/${d##*/}/name); case "$n" in imx586*) main=$d;; esac; done
v4l2-ctl -d "$main" --set-ctrl=test_pattern=1
trap 'v4l2-ctl -d "$main" --set-ctrl=test_pattern=0' EXIT
cat /proc/sys/kernel/random/boot_id > "$base/stress.boot-before"
/home/user/verify-stripe-v4l2 9000 /home/user/stripe-reference.raw > "$base/stress-mmap.log" 2>&1 &
capture=$!
while kill -0 "$capture" 2>/dev/null; do
 hottest=0
 for f in /sys/class/thermal/thermal_zone*/type; do
  name=$(cat "$f")
  case "$name" in cpu*-thermal|cluster*-thermal|camera-thermal)
   t=$(cat "${f%type}temp"); [ "$t" -le "$hottest" ] || hottest=$t;;
  esac
 done
 echo "$(cut -d ' ' -f1 /proc/uptime) $hottest" >> "$base/stress-mmap-thermal.log"
 if [ "$hottest" -ge 80000 ]; then kill "$capture"; echo THERMAL_STOP >> "$base/stress-mmap-thermal.log"; break; fi
 sleep 5
done
rc=0; wait "$capture" || rc=$?
checkrc=$rc
cat /proc/sys/kernel/random/boot_id > "$base/stress.boot-after"
echo "capture_rc=$rc checker_rc=$checkrc" > "$base/stress.done"

dmesg > "$base/stress.dmesg"
