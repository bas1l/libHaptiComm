#!/bin/sh

sudo apt-get update;sudo apt-get dist-upgrade;
sudo apt-get install pavucontrol linux-sound-base alsa-base alsa-utils lightdm ubuntu-desktop  linux-image-`uname -r` libasound2;
sudo apt-get -y --reinstall install linux-sound-base alsa-base alsa-utils lightdm ubuntu-desktop  linux-image-`uname -r` libasound2;
killall pulseaudio;
rm -r ~/.pulse*;
ubuntu-support-status;
sudo usermod -aG `cat /etc/group | grep -e '^pulse:' -e '^audio:' -e '^pulse-access:' -e '^pulse-rt:' -e '^video:' | awk -F: '{print $1}' | tr '\n' ',' | sed 's:,$::g'` `whoami`

sudo apt-get install pulseaudio

sudo apt-get install libpulse-dev

sudo apt-get install osspd
