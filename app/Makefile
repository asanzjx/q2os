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


all: test_app
	$(OBJCOPY) -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary test_app test_app.bin
	rm -rf *.s~ *.s *.S~ *.c~ *.h~ Makefile~ User.lds~ test_app

test_app: test.o userlibs.o syscall.o
	$(LD) -b elf64-x86-64 -z muldefs -o test_app userlibs.o test.o syscall.o -T User.lds

test.o: test.c
	$(GCC) $(CFLAGS) -c test.c

clean:
	rm -rf *.o *.s~ *.s *.S~ *.c~ *.h~ Makefile~ User.lds~ test_app test_app.bin

