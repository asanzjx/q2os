nasm -o ./loader.bin ./loader.asm
sudo mount ../deploy/q2os_floppy.img /media/ -t vfat -o loop
sudo cp ./loader.bin /media/
sudo sync
sudo umount /media/
