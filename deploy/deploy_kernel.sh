sudo mount ./q2os_floppy.img /media/ -t vfat -o loop
sudo cp ../kernel.bin /media/
sudo sync
sudo umount /media/
bochs -q  -f ./q2os_bochs_floppy
