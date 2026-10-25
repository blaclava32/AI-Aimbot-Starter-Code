# SparkOS

A tiny hobby operating system you can build and boot in a VM.

It is a 32-bit x86 kernel with:

- **Multiboot2** boot via GRUB
- **VGA text** console (80×25)
- **PS/2 keyboard** input (polled)
- An interactive **shell** (`help`, `echo`, `about`, `peek` / `poke`, …)

This is for learning and fun — not a real OS for daily use.

## Requirements

On Ubuntu / Debian (including most cloud VMs):

```bash
sudo apt-get update
sudo apt-get install -y build-essential nasm qemu-system-x86 xorriso grub-pc-bin grub-common
```

You need a 32-bit capable toolchain (`gcc -m32`). If `gcc` cannot find 32-bit libs:

```bash
sudo apt-get install -y gcc-multilib
```

## Build

```bash
cd hobby-os
make
```

That produces `sparkos.iso`.

## Run in QEMU (recommended)

```bash
make run
```

Or manually:

```bash
qemu-system-x86_64 -cdrom sparkos.iso -m 64M
```

You should see the SparkOS banner and a `spark:~$` prompt. Try:

```
help
about
echo hello from my OS
color 14
clear
```

## Run on VirtualBox / VMware / your VM

1. Build `sparkos.iso` on this machine (`make` in `hobby-os/`).
2. Create a new VM (or use an existing one) with:
   - Type: **Other** / **Other/Unknown**, or Linux is fine
   - Memory: **64 MB** is enough
   - No hard disk required — boot from the ISO
3. Attach `hobby-os/sparkos.iso` as a **CD/DVD** and boot from it.
4. On first boot, GRUB loads `SparkOS` automatically.

If the host is already a Linux VM and you want to boot SparkOS *inside* it, use QEMU nested (needs nested virt) or burn/attach the ISO as a second machine.

## Project layout

```
hobby-os/
  boot/boot.asm      Multiboot2 header + stack + jump to C
  kernel/            Kernel, VGA, keyboard, shell
  grub/grub.cfg      GRUB menu
  linker.ld          Link at 1 MiB
  Makefile
```

## Ideas to extend

- Enable interrupts + a PIT timer for a real uptime clock
- Add a simple physical memory allocator
- Draw in VGA graphics mode (320×200)
- Load a second “userspace” ELF from a ramdisk
- Port a tiny filesystem (e.g. read-only tar)

Happy hacking.
