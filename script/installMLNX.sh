#!/bin/bash

if [ ! -d "tmp" ]; then
	mkdir tmp
fi

cd tmp
wget https://www.mellanox.com/page/mlnx_ofed_eula?mtag=linux_sw_drivers&mrequest=downloads&mtype=ofed&mver=MLNX_OFED-5.4-3.7.5.0&mname=MLNX_OFED_LINUX-5.4-3.7.5.0-ubuntu22.04-x86_64.tgz
tar -xvf MLNX_OFED_LINUX-5.4-3.7.5.0-ubuntu22.04-x86_64.tgz
cd MLNX_OFED_LINUX-5.4-3.7.5.0-ubuntu22.04-x86_64

sudo ./mlnxofedinstall  --force
sudo /etc/init.d/openibd restart
sudo /etc/init.d/opensmd restart