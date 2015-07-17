#!/bin/bash

set -e

cd /
apt-get update
apt-get upgrade -y
apt-get install -y mercurial wget autoconf automake libtool gettext build-essential libpcre3-dev
hg clone https://crack-language.googlecode.com/hg/ crack
wget http://llvm.org/releases/3.3/llvm-3.3.src.tar.gz
tar -xvzf llvm-3.3.src.tar.gz

# Build LLVM.
cd /llvm-3.3.src
./configure
make REQUIRES_RTTI=1
make install

# Build crack from head.
cd /crack
./bootstrap
./configure
make
make install
ldconfig

# Clean up all of the development cruft.
rm -r /crack
rm -r /llvm-3.3.src*
apt-get remove -y mercurial wget autoconf automake libtool gettext build-essential libpcre3-dev
