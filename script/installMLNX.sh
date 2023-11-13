#!/bin/bash

if [ ! -d "tmp" ]; then
	mkdir tmp
fi

cd tmp
wget https://content.mellanox.com/ofed/MLNX_OFED-5.4-3.7.5.0/MLNX_OFED_LINUX-5.4-3.7.5.0-ubuntu22.04-x86_64.tgz
tar -xvf MLNX_OFED_LINUX-5.4-3.7.5.0-ubuntu22.04-x86_64.tgz
cd MLNX_OFED_LINUX-5.4-3.7.5.0-ubuntu22.04-x86_64

sudo ./mlnxofedinstall  --force
sudo /etc/init.d/openibd restart
sudo /etc/init.d/opensmd restart