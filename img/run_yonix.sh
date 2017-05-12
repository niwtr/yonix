#!/bin/sh

QEMU=qemu-system-i386

dd if=/dev/zero of=swap.img bs=1M count=200
mkswap swap.img

CPUS=1
# QEMUOPTS=-drive file=fs.img,index=1,media=disk,format=raw -drive file=yonix.img,index=0,media=disk,format=raw -smp $(CPUS) -m 512 $(QEMUEXTRA) -k en-us
QEMUOPTS="-drive file=swap.img,index=2,format=raw -drive file=fs.img,index=1,format=raw -drive file=yonix.img,index=0,format=raw -smp $CPUS -m 512"
$QEMU -serial mon:stdio $QEMUOPTS