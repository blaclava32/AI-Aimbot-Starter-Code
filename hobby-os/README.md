# SparkOS 1.0

A **complete hobby operating system** for x86 — bootable in QEMU, VirtualBox, VMware, or any PC VM.

Not Linux and not Windows — a self-contained teaching OS with real kernel subsystems you can read and change.

## Features

| Subsystem | What it does |
|-----------|----------------|
| **Boot** | Multiboot2 + GRUB ISO |
| **GDT / IDT** | Flat protected-mode segments, exception + IRQ gates |
| **PIC + PIT** | Hardware IRQs, 100 Hz system timer, uptime |
| **Keyboard** | PS/2 IRQ1 ring buffer |
| **PMM + heap** | 4 KiB frame allocator + `kmalloc`/`kfree` |
| **VFS** | In-memory Unix-like tree (`/etc`, `/home/spark`, …) |
| **Tasks** | Multi-tasking with `ps`, `spawn`, `kill`, `sleep` |
| **Shell** | Login, prompt, editor, calc, file tools |

## Quick start

```bash
sudo apt-get install -y build-essential nasm qemu-system-x86 \
  xorriso grub-pc-bin grub-common gcc-multilib

cd hobby-os
make
make run
```

At the login prompt press Enter, then try:

```
help
ls /
cat /etc/motd
cd /home/spark
edit hello.txt
free
ps
spawn worker
uname
uptime
```

## Run on your VM

1. `make` → produces `sparkos.iso`
2. Create a VM (64 MB RAM is enough) **or** attach the ISO to an existing one
3. Boot from the CD — GRUB loads SparkOS automatically

## Layout

```
hobby-os/
  boot/boot.asm       Multiboot2 entry + stack
  kernel/
    gdt/idt/pic/pit   CPU + interrupt stack
    irq/isr           Dispatch + stubs
    keyboard/vga      Console I/O
    heap/pmm          Memory managers
    vfs               Filesystem
    task/switch       Scheduler
    shell             Login + command processor
    kernel.c          Init / boot log
  grub/grub.cfg
  Makefile
```

## Honest scope

SparkOS is a **finished mini-OS**: it boots, manages hardware, memory, files, and processes, and gives you an interactive userland. It is not a daily driver (no networking, no disk persistence, no ELF loader yet).

Great next upgrades: ATA disk + FAT, paging, ELF user programs, TCP stack.
