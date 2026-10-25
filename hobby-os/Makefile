# SparkOS 1.0 — complete hobby OS

CC      := gcc
AS      := nasm
LD      := ld

CFLAGS  := -m32 -ffreestanding -fno-builtin -fno-stack-protector \
           -fno-pic -fno-pie -Wall -Wextra -Werror=implicit-function-declaration \
           -O2 -std=c11 -I kernel
ASFLAGS := -f elf32
LDFLAGS := -m elf_i386 -T linker.ld -nostdlib

KERNEL_OBJS := \
	boot/boot.o \
	kernel/isr.o \
	kernel/switch.o \
	kernel/gdt.o \
	kernel/idt.o \
	kernel/pic.o \
	kernel/pit.o \
	kernel/irq.o \
	kernel/vga.o \
	kernel/keyboard.o \
	kernel/string.o \
	kernel/heap.o \
	kernel/pmm.o \
	kernel/vfs.o \
	kernel/task.o \
	kernel/shell.o \
	kernel/kernel.o

ISO     := sparkos.iso
KERNEL  := build/spark.bin
ISODIR  := build/iso

.PHONY: all clean run iso dirs

all: iso

dirs:
	mkdir -p build boot kernel $(ISODIR)/boot/grub

boot/boot.o: boot/boot.asm
	$(AS) $(ASFLAGS) $< -o $@

kernel/isr.o: kernel/isr.asm
	$(AS) $(ASFLAGS) $< -o $@

kernel/switch.o: kernel/switch.asm
	$(AS) $(ASFLAGS) $< -o $@

kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL): dirs $(KERNEL_OBJS) linker.ld
	mkdir -p build
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJS)

iso: $(KERNEL) grub/grub.cfg
	mkdir -p $(ISODIR)/boot/grub
	cp $(KERNEL) $(ISODIR)/boot/spark.bin
	cp grub/grub.cfg $(ISODIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) $(ISODIR)

run: iso
	qemu-system-x86_64 -cdrom $(ISO) -m 64M -serial stdio

clean:
	rm -rf build boot/*.o kernel/*.o $(ISO)
