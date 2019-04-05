#!/bin/bash

abspath=$(pwd)
tmp="tmp"
abstmp="$(pwd)/$tmp"
sphinxbase="sphinxbase-5prealpha"
sphinxpocket="pocketsphinx-5prealpha"

apt-get update
apt-get upgrade

[ ! -d "$dtmp" ] && mkdir "$dtmp"
cd "$abstmp"

# Boost libraries
apt-get install -y libboost-all-dev
apt-get install -y libboost1.62-all-dev

# CMU Sphinx (pocket+base)
apt-get install -y gcc automake autoconf libtool bison swig pulseaudio python3.5-dev
[ ! -f "$sphinxbase.tar.gz" ] && wget "https://sourceforge.net/projects/cmusphinx/files/sphinxbase/5prealpha/$sphinxbase.tar.gz" && tar -zxvf "$sphinxbase.tar.gz"
[ ! -f "$sphinxpocket.tar.gz" ] && wget "https://sourceforge.net/projects/cmusphinx/files/pocketsphinx/5prealpha/$sphinxpocket.tar.gz" && tar -zxvf "$sphinxpocket.tar.gz"
cd "$abstmp/$sphinxbase"
./autogen.sh
./configure
make
sudo make install
export LD_LIBRARY_PATH="/usr/local/lib"
export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig"
cd "$abstmp/$sphinxpocket"
./autogen.sh
./configure
make
sudo make install

# WiringPi Library
cd "$abstmp"
apt-get purge wiringpi
hash -r
apt-get install -y git-core
apt-get update
apt-get upgrade
git clone git://git.drogon.net/wiringPi
cd "$abstmp/wiringPi"
git pull origin
./build



# Create binary files
mkdir "$abspath/../build"
cd "$abspath/../build"
cmake ../libHaptiComm
make



