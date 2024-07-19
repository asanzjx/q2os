# Boot: Bios / UEFI
BOOT = Bios

# PC / Bochs / Qemu
MACHINE := Bochs

# APIC / 8259A
PIC := APIC
# PIC := PIC

WORKDIR := $(shell pwd)

OLDCFLAGS := -mcmodel=large -fno-builtin -m64
NEWCFLAGS := -fno-stack-protector -fno-pie -fno-pic -fno-common -std=gnu89 -nostartfiles -Wno-address-of-packed-member
CFLAGS := -mcmodel=large -fno-builtin -m64 -fno-stack-protector -fno-pie -fno-pic -fno-common -std=gnu89 -nostartfiles -Wno-address-of-packed-member  -D$(PIC) -D$(BOOT) -D$(MACHINE)

ifeq ($(shell arch),x86_64)
	GCC = gcc
	as = as
	LD = ld
	OBJCOPY = objcopy
else
	GCC = x86_64-elf-gcc
	AS = x86_64-elf-as
	LD = x86_64-elf-ld
	OBJCOPY = x86_64-elf-objcopy
	OBJDUMP := x86_64-elf-objdump
endif

all: system
	$(OBJCOPY) -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary system kernel.bin
	rm -rf *.o *.as
	$(OBJDUMP) -d ./system > system.bin.asm
# make -C $(WORKDIR)/deploy
	cd $(WORKDIR)/deploy && ./deploy_kernel.sh 


ifeq ($(PIC),APIC)
system:	head.o entry.o APU_boot.o printk.o trap.o memory.o PIC.o interrupt.o task.o keyboard.o mouse.o disk.o SMP.o HPET.o schedule.o main.o
	$(LD) -b elf64-x86-64 -z muldefs -o system head.o entry.o APU_boot.o trap.o printk.o memory.o PIC.o interrupt.o task.o keyboard.o mouse.o disk.o SMP.o HPET.o schedule.o main.o -T Kernel.lds
else
system:	head.o entry.o printk.o trap.o memory.o PIC.o task.o main.o
	$(LD) -b elf64-x86-64 -z muldefs -o system head.o entry.o trap.o printk.o memory.o PIC.o task.o main.o -T Kernel.lds
endif

APU_boot.o: APU_boot.S
	$(GCC) -E APU_boot.S > APU_boot.as
	$(AS) --64 -o APU_boot.o APU_boot.as

head.o:	kernel_header.S
	$(GCC) -E  kernel_header.S > kernel_header.as
	$(AS) --64 -o head.o kernel_header.as

entry.o: entry.S
	$(GCC) -E  entry.S > entry.as
	$(AS) --64 -o entry.o entry.as

main.o:	main.c
	$(GCC) $(CFLAGS) -c main.c

printk.o: printk.c
	$(GCC) $(CFLAGS) -c printk.c

trap.o: trap.c
	$(GCC) $(CFLAGS) -c trap.c

memory.o: memory.c
	$(GCC) $(CFLAGS) -c memory.c

interrupt.o: interrupt.c
	$(GCC) $(CFLAGS) -c interrupt.c

ifeq ($(PIC),APIC)
PIC.o: APIC.c
	$(GCC) $(CFLAGS) -c APIC.c -o PIC.o
keyboard.o: keyboard.c
	$(GCC)  $(CFLAGS) -c keyboard.c

mouse.o: mouse.c
	$(GCC)  $(CFLAGS) -c mouse.c

disk.o: disk.c
	$(GCC)  $(CFLAGS) -c disk.c
else
PIC.o: 8259A.c
	$(GCC) $(CFLAGS) -c 8259A.c -o PIC.o
endif

task.o: task.c
	$(GCC) $(CFLAGS) -c task.c

SMP.o: SMP.c
	$(GCC) $(CFLAGS) -c SMP.c

HPET.o: HPET.c
	$(GCC) $(CFLAGS) -c HPET.c

schedule.o: schedule.c
	$(GCC) $(CFLAGS) -c schedule.c

loader:
	nasm ./loader.asm -o ./loader.bin

clean:
	rm -rf *.o *.s system* kernel.bin*

test:
	@echo $(MAKE_VERSION)
	@echo $(shell arch)
	$(GCC) -v

