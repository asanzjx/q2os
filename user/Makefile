CFLAGS := -mcmodel=large -fno-builtin -m64 -fno-stack-protector -fno-pie -fno-pic -fno-common -std=gnu89 -nostartfiles -Wno-address-of-packed-member


ifeq ($(shell arch),x86_64)
	GCC = gcc
	as = as
	LD = ld
	OBJCOPY = objcopy
	OBJDUMP := objdump
	NM = nm
else
	GCC = x86_64-elf-gcc
	AS = x86_64-elf-as
	LD = x86_64-elf-ld
	OBJCOPY = x86_64-elf-objcopy
	OBJDUMP := x86_64-elf-objdump
	NM = x86_64-elf-nm
endif


all: user_init
	$(OBJCOPY) -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary user_init init.bin
	cp ./userlibs.o ../app/
	cp ./syscall.o ../app/
	$(OBJDUMP) -d ./user_init > init.bin.asm
	rm -rf *.o *.s~ *.s *.S~ *.c~ *.h~ Makefile~ User.lds~

user_init: init.o userlibs.o syscall.o keyboard.o shell.o
	$(LD) -b elf64-x86-64 -z muldefs -o user_init init.o userlibs.o syscall.o keyboard.o shell.o -T User.lds

init.o: init.c
	$(GCC)  $(CFLAGS) -c init.c

userlibs.o: userlibs.c
	$(GCC)  $(CFLAGS) -c userlibs.c

syscall.o: syscall.c
	$(GCC)  $(CFLAGS) -c syscall.c

keyboard.o: keyboard.c
	$(GCC)  $(CFLAGS) -c keyboard.c

shell.o: shell.c
	$(GCC)  $(CFLAGS) -c shell.c
	
clean:
	rm -rf *.o *.s~ *.s *.S~ *.c~ *.h~ Makefile~ User.lds~ user_init init.bin init.bin.asm

