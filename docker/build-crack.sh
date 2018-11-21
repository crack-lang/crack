#!/bin/bash

set -e

cd /
apt-get update
apt-get upgrade -y
apt-get install -y git wget autoconf automake libtool gettext build-essential libpcre3-dev python libssl-dev libfuse-dev
git clone https://github.com/crack-lang/crack
wget http://llvm.org/releases/3.3/llvm-3.3.src.tar.gz
tar -xvzf llvm-3.3.src.tar.gz

# Build LLVM.
cd /llvm-3.3.src
./configure
make -j5 REQUIRES_RTTI=1
make install

# Build crack from head.
cd /crack
./bootstrap
./configure
make -j5
make install
ldconfig

# Clean up all of the development cruft.
rm -r /crack
rm -r /llvm-3.3.src*
apt-get remove -y mercurial wget autoconf automake libtool gettext build-essential libpcre3-dev
