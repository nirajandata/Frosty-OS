i386-elf-gcc -std=gnu99 -ffreestanding -g -c start.s -o start.o
i386-elf-gcc -std=gnu99 -ffreestanding -g -c kernel.c -o kernel.o
i386-elf-gcc -ffreestanding -nostdlib -g -T linker.ld start.o kernel.o -o mykernel.elf -lgcc

