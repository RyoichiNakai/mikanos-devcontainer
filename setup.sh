#!/bin/bash

# なぜかシェルだと通らない
# remote containerに入った際に、以下のコマンドを実行しておく
# cd ~/edk2
# source edksetup.sh
# cd /workspaces/mikanos-devcontainer/

OS_DIR=/workspaces/mikanos-devcontainer/mymikanos
cp config/target.txt ~/edk2/Conf/target.txt 
cp config/tools_def.txt ~/edk2/Conf/tools_def.txt
ln -sf /workspaces/mikanos-devcontainer/mymikanos/MikanLoaderPkg ~/edk2/
