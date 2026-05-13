#!/bin/sh

set -e

name=$1
addr=10.100.4.202

scp "${name}.bit.bin" "${name}.dtbo" root@${addr}:/lib/firmware
scp src/kmd/strela/strela.ko root@${addr}:/root
scp src/umd/linux/strela_test_linux.elf root@${addr}:/root

# Assuming configfs is mounted
# mount -t configfs none /sys/kernel/config
ssh root@${addr} << EOF
	set -x

	rmmod strela

	if [ -d /sys/kernel/config/device-tree/overlays/strela ]
	then
		rmdir /sys/kernel/config/device-tree/overlays/strela
	fi

	mkdir -p /sys/kernel/config/device-tree/overlays/strela
	echo "${name}.dtbo" >/sys/kernel/config/device-tree/overlays/strela/path
	cat /sys/kernel/config/device-tree/overlays/strela/status

	insmod strela.ko

	ls /sys/class/strela
	grep strela /proc/devices
	grep axi_lite /proc/iomem
	ls /dev/strela*

	dmesg | tail -n 20
	./strela_test_linux.elf
EOF
