# q2os
 a study os


# develop env and tools

- wsl2 ubuntu 22.04 LTS(gcc version 11.4.0) / MacOS
- bochs 2.8

```bochs 2.8 compile
// Compile configure：
./configure --with-x11 --with-wx --enable-debugger --enable-disasm \
 --enable-all-optimizations --enable-readline --enable-long-phy-address \
 --enable-ltdl-install --enable-idle-hack --enable-plugins --enable-a20-pin \
 --enable-x86-64 --enable-smp --enable-cpu-level=6 --enable-large-ramfile \
 --enable-repeat-speedups --enable-fast-function-calls   \
 --enable-handlers-chaining  --enable-trace-linking \
 --enable-configurable-msrs --enable-show-ips --enable-cpp \
 --enable-debugger-gui --enable-iodebug --enable-logging \
 --enable-assert-checks --enable-fpu --enable-vmx=2 --enable-svm \
 --enable-3dnow --enable-alignment-check  --enable-monitor-mwait \
 --enable-avx  --enable-evex --enable-x86-debugger --enable-pci \
 --enable-voodoo

cp misc/bximage.cpp misc/bximage.cc
cp iodev/hdimage/hdimage.cpp iodev/hdimage/hdimage.cc 
cp iodev/hdimage/vmware3.cpp iodev/hdimage/vmware3.cc
cp iodev/hdimage/vmware4.cpp iodev/hdimage/vmware4.cc
cp iodev/hdimage/vpc-img.cpp iodev/hdimage/vpc-img.cc
cp iodev/hdimage/vbox.cpp iodev/hdimage/vbox.cc

```

# Feature

implement based on Single CPU
- Advanced memory manage, based on SLAB
- APIC, based on Local APIC and I/O APIC
- keyboard, mouse and disk(block device model) driver based on APIC
- task, user level function, system call

// todo..
[] user function ret
[] support multi cpus, task schedule
...

