<p align="center">
  <a href="https://github.com/SergeyIvanovDevelop/Simple-OS">
    <img alt="Simple-OS" src="./resources/logo.png" width="280" height="280" />
  </a>
</p>
<h1 align="center">
  Simple OS
</h1>

## Simple OS &middot; [![GitHub license](https://img.shields.io/badge/license-CC%20BY--NC--SA%203.0-blue)](./LICENSE) [![C](https://img.shields.io/badge/language-C-yellow)](https://www.iso.org/standard/74528.html) [![Assembler](https://img.shields.io/badge/assembler-NASM-9cf)](https://www.nasm.us/) [![Bootloader](https://img.shields.io/badge/bootloader-GRUB%2FGRUB2-lightgrey)](https://www.gnu.org/software/grub/) [![LinkedIn](https://img.shields.io/badge/linkedin-Sergey%20Ivanov-blue)](https://www.linkedin.com/in/sergey-ivanov-33413823a/) [![Telegram](https://img.shields.io/badge/telegram-%40SergeyIvanov__dev-blueviolet)](https://t.me/SergeyIvanov_dev) ##

This repository contains code for a native implementation of a simple operating system for devices with an `Intel` processor and `x86` and `x86_64` architectures.

Main implemented functionality:
- Handling keyboard keystrokes
- Tracking cursor movements
- Ability to store data in memory (and issue on request)
- Ability to display information (including in color) on the screen (by accessing the address space of the video memory)


## :computer: Getting Started  ##

**Clone repository**

1. Go to home directory and clone repository from github: `cd ~ && git clone https://SergeyIvanovDevelop@github.com/SergeyIvanovDevelop/Simple-OS`

**Run in QEMU**<br>

_Note: for this you need to collect using "kernel_1.asm" and "link_1.ld"_

2. run in terminal: `qemu-system-i386 -kernel kernel`

**Running in VirtualBox or on a real machine**<br>

_Note: If you run on a real machine, then you need to boot from the `BIOS`, not from `UEFI` (`Legacy` - not suitable)._

3. It is necessary to add an item to `/boot/grub/grub.cfg`:<br>

```
menuentry "Simple_OS_by_SS" {
	multiboot2 /boot/kernel-45
	boot
}
```

4. Direct assembly of the kernel: `make clear && make`

_Note: It is necessary to check that the `/boot/` folder actually contains the kernel file `kernel-45`_

5. It is necessary to configure `grub` so that its `timeout` is not equal to zero (or press `Esc` at boot) - in order to be able to select the kernel of our OS for loading

6. After you need to restart the computer, enter the `GRUB` (or `GRUB 2`) menu and select (select where you add it in the `/boot/grub/grub.cfg` file) an operating system called `Simple_OS_by_SS`

**:clapper: Example using (GIF):**<br>

This animation demonstrates scenarios for using the `Simple OS`.<br>
<p align="center">
  <img src="./resources/Simple-OS.gif" alt="animated" />
</p>

### :bookmark_tabs: Licence ###
Simple OS is [CC BY-NC-SA 3.0 licensed](./LICENSE).
