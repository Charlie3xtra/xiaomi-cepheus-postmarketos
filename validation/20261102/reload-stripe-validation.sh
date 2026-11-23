set -eu
systemctl stop cepheus-camera-ui.service
mkdir -p /home/user/stripe-validation
for round in 0 1 2 3 4 5 6 7 8 9 10; do
 rmmod cepheus_aux imx586 qcom_camss
 if [ "$round" = 0 ]; then modprobe qcom_camss; else insmod /home/user/qcom-camss-fix.ko; fi
 modprobe imx586
 modprobe cepheus_aux
 for d in /dev/v4l-subdev*; do n=$(cat /sys/class/video4linux/${d##*/}/name); case "$n" in imx586*) main=$d;; esac; done
 v4l2-ctl -d "$main" --set-ctrl=test_pattern=1
 for open in 1 2; do
  bash /home/user/capture-extra.sh 'imx586 2-001a' 1 4000 3000 SRGGB10_1X10 pRAA /home/user/stripe-validation/last.raw 3 > /home/user/stripe-validation/$round-$open.log 2>&1
  echo "reload=$round open=$open"
  python3 /home/user/verify-stripe-stream.py /home/user/stripe-validation/last.raw || [ "$round" = 0 ]
 done
 v4l2-ctl -d "$main" --set-ctrl=test_pattern=0
done
