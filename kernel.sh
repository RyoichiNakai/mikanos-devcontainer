#!/bin/bash

# 以下を実行しておく
# source $HOME/osbook/devenv/buildenv.sh

cd /workspaces/mikanos-devcontainer/mymikanos/kernel
clang++ $CPPFLAGS -O2 -Wall -g --target=x86_64-elf -fno-exceptions -ffreestanding -c main.cpp
ld.lld $LDFLAGS --entry KernelMain -z norelro --image-base 0x100000 --static -o kernel.elf main.o
