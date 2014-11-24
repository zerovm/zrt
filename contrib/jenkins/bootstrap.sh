#!/usr/bin/env bash
export ZVM_PREFIX=$HOME/zvm-root
export ZRT_ROOT=$HOME/zrt
export LD_LIBRARY_PATH=/usr/lib64
export CPATH="/usr/x86_64-nacl/include:$HOME/libffi/x86_64-pc-nacl/include"
export PATH="$PATH:$ZVM_PREFIX/bin"

# Copy the current clone of zrt from the shared dir to another working
# directory
rsync -az --exclude=contrib/jenkins/.* /host-workspace/ $ZRT_ROOT

# Make zerovm available in path
ln -s `which zerovm` $ZVM_PREFIX/bin/zerovm

cd $ZRT_ROOT
make
