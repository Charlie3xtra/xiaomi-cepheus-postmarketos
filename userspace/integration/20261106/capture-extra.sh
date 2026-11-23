#!/bin/sh
set -eu
sensor=$1
phy=$2
w=$3
h=$4
code=$5
pix=$6
out=$7
count=${8:-3}
media-ctl -d /dev/media0 -r
media-ctl -d /dev/media0 -l "\"msm_csiphy$phy\":1->\"msm_csid0\":0[1]"
media-ctl -d /dev/media0 -l '"msm_csid0":1->"msm_vfe0_rdi0":0[1]'
for e in "\"$sensor\":0" "\"msm_csiphy$phy\":0" '"msm_csid0":0' '"msm_csid0":1' '"msm_vfe0_rdi0":0'; do
    media-ctl -d /dev/media0 -V "$e [fmt:$code/$w"x"$h]"
done
v4l2-ctl -d /dev/video0 --set-fmt-video "width=$w,height=$h,pixelformat=$pix"
v4l2-ctl -d /dev/video0 --get-fmt-video
timeout -k 1 5 v4l2-ctl -d /dev/video0 --stream-mmap --stream-count="$count" --stream-to="$out" --verbose
stat -c '%s bytes' "$out"
dmesg | tail -9
