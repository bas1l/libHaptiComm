#!/bin/bash

tmppath="$(pwd)/install"
buildpath="$(pwd)/../build"

sphinxbase="sphinxbase-5prealpha"
sphinxpocket="pocketsphinx-5prealpha"


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
#sudo apt-get install -y gcc automake autoconf libtool swig python3.5-dev
#[ ! -f "$sphinxbase.tar.gz" ] && wget --no-check-certificate "https://sourceforge.net/projects/cmusphinx/files/sphinxbase/5prealpha/$sphinxbase.tar.gz" && tar -zxvf "$sphinxbase.tar.gz"
#[ ! -f "$sphinxpocket.tar.gz" ] && wget --no-check-certificate "https://sourceforge.net/projects/cmusphinx/files/pocketsphinx/5prealpha/$sphinxpocket.tar.gz" && tar -zxvf "$sphinxpocket.tar.gz"
#cd "$tmppath/$sphinxbase"
#sudo ./autogen.sh
#sudo ./configure
#sudo make
#sudo make install
#export LD_LIBRARY_PATH="/usr/local/lib"
#export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig"
#cd "$tmppath/$sphinxpocket"
#sudo ./autogen.sh
#sudo ./configure
#sudo make
#sudo make install

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
#make
