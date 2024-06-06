NEWCFLAGS := -fno-stack-protector -fno-pie -fno-pic -fno-common -std=gnu89 -nostartfiles -Wno-address-of-packed-member

all: system
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary system kernel.bin
	rm -rf *.o *.s

system:	head.o entry.o printk.o trap.o memory.o interrupt.o task.o main.o
	ld -b elf64-x86-64 -z muldefs -o system head.o entry.o trap.o printk.o memory.o interrupt.o task.o main.o -T Kernel.lds

head.o:	kernel_header.S
	gcc -E  kernel_header.S > kernel_header.s
	as --64 -o head.o kernel_header.s

entry.o: entry.S
	gcc -E  entry.S > entry.s
	as --64 -o entry.o entry.s

main.o:	main.c
	gcc -mcmodel=large -fno-builtin -m64 -c main.c $(NEWCFLAGS)

printk.o: printk.c
	gcc -mcmodel=large -fno-builtin -m64 -c printk.c $(NEWCFLAGS)

trap.o: trap.c
	gcc -mcmodel=large -fno-builtin -m64 -c trap.c

memory.o: memory.c
	gcc  -mcmodel=large -fno-builtin -m64 -c memory.c

interrupt.o: interrupt.c
	gcc  -mcmodel=large -fno-builtin -m64 -c interrupt.c

task.o: task.c
	gcc  -mcmodel=large -fno-builtin -m64 -c task.c	$(NEWCFLAGS)

loader:
	nasm ./loader.asm -o ./loader.bin

clean:
	rm -rf *.o *.s system kernel.bin


