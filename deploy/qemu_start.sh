#!/bin/bash

qemu-system-x86_64 -m 2048 \
    -fda ./q2os_floppy.img

    # -nographic
    # -drive file=./q2os_floppy.img,format=raw,if=pflash -boot a