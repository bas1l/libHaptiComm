#!/bin/bash

tmppath="$(pwd)/install"
buildpath="$(pwd)/../build"

sudo apt-get update
sudo apt-get upgrade

[ ! -d "$tmppath" ] && mkdir "$tmppath"
cd "$tmppath"

# Boost libraries
sudo apt-get install -y libboost-all-dev
sudo apt-get install -y libboost1.62-all-dev

# CMU Sphinx (pocket+base)
sudo apt-get remove pulseaudio -y
sudo aptitude purge pulseaudio -y
sudo mv /usr/include/pulse/pulseaudio.h /usr/include/pulse/pulseaudio.h.old
sudo apt-get install -y alsa-utils bison libasound2-dev cmake libncurses5-dev

# WiringPi Library
cd "$tmppath"
sudo apt-get purge wiringpi
hash -r
sudo apt-get install -y git-core
sudo apt-get update
sudo apt-get upgrade
git clone git://git.drogon.net/wiringPi
cd "$tmppath/wiringPi"
git pull origin
sudo ./build


# Create binary files
[ ! -d $buildpath ] && mkdir "$buildpath"
cd "$buildpath"
cmake ../libHaptiComm

cp braille.sh ../ braille.sh
cp haptiComm.sh ../haptiComm.sh
#make
