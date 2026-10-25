# SparkOS — tiny hobby OS build

CC      := gcc
AS      := nasm
LD      := ld

CFLAGS  := -m32 -ffreestanding -fno-builtin -fno-stack-protector \
           -fno-pic -fno-pie -Wall -Wextra -O2 -std=c11
ASFLAGS := -f elf32
LDFLAGS := -m elf_i386 -T linker.ld -nostdlib

KERNEL_OBJS := \
	boot/boot.o \
	kernel/kernel.o \
	kernel/vga.o \
	kernel/keyboard.o \
	kernel/shell.o \
	kernel/string.o

ISO     := sparkos.iso
KERNEL  := build/spark.bin
ISODIR  := build/iso

.PHONY: all clean run run-nographic iso dirs

all: iso

dirs:
	mkdir -p build boot kernel $(ISODIR)/boot/grub

boot/boot.o: boot/boot.asm
	$(AS) $(ASFLAGS) $< -o $@

kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -I kernel -c $< -o $@

$(KERNEL): dirs $(KERNEL_OBJS) linker.ld
	mkdir -p build
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJS)

iso: $(KERNEL) grub/grub.cfg
	mkdir -p $(ISODIR)/boot/grub
	cp $(KERNEL) $(ISODIR)/boot/spark.bin
	cp grub/grub.cfg $(ISODIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) $(ISODIR) 2>/dev/null || \
	  grub-mkrescue -o $(ISO) $(ISODIR)

run: iso
	qemu-system-x86_64 -cdrom $(ISO) -m 64M -serial stdio

# Headless smoke test: boot, then quit after a few seconds
run-nographic: iso
	qemu-system-x86_64 -cdrom $(ISO) -m 64M -display none -serial stdio \
	  -no-reboot -no-shutdown -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
	  || true

clean:
	rm -rf build boot/*.o kernel/*.o $(ISO)
