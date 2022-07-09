#!/bin/bash

# source $HOME/osbook/devenv/buildenv.sh
cd /workspaces/mikanos-devcontainer/mymikanos/kernel
make hankaku.bin
make hankaku.o
make
build
make clean
