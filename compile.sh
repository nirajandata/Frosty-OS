i386-elf-as start.s -o start.o
i386-elf-gcc -c kernel.c -o kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i386-elf-gcc -std=gnu99 -ffreestanding -g -c kernel.c -o kernel.o
i386-elf-gcc -T linker.ld -o myos.bin -ffreestanding -O2 -nostdlib start.o kernel.o -lgcc

