#!/bin/bash
system=`uname`

if [ "$system" == "Darwin" ]; then
    hdiutil attach ./q2os_floppy.img -mountpoint ./fat_mount
    cp ../kernel.bin ./fat_mount
    sync
    hdiutil detach ./fat_mount
    hdiutil attach ./q2os_ata_fat32.img -mountpoint ./fat_mount
    cp ../user/init.bin ./fat_mount
    cp ../app/test_app.bin ./fat_mount
    # cp ./KEYBOARD.DEV ./fat_mount
    sync
    hdiutil detach ./fat_mount
    bochs -q  -f ./mac_q2os_bochs_floppy
else
    echo $system
    sudo mount ./q2os_floppy.img /media/ -t vfat -o loop
    sudo cp ../kernel.bin /media/
    sudo sync
    sudo umount /media/
    sudo mount ./q2os_ata_fat32.img /media/ -t vfat -o loop
    sudo cp ../user/init.bin /media/
    sudo cp ../app/test_app.bin /media/
    # sudo rm /media/KEYBOARD.DEV
    # sudo cp ./KEYBOARD.DEV /media/
    sudo sync
    sudo umount /media/
    bochs -q  -f ./q2os_bochs_floppy
fi

