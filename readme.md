# RadiumOS

<div align="center">
  <img src="https://raw.githubusercontent.com/RadiumOS-Dev/RadiumOS/main/images/radiumOS.png" alt="RadiumOS Logo" width="150"/>
  
  [![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
  [![Status](https://img.shields.io/badge/status-experimental-red.svg)]()

  [![C](https://img.shields.io/badge/C-00599C?style=flat&logo=c&logoColor=white)]()
  [![Assembly](https://img.shields.io/badge/Assembly-654FF0?style=flat&logo=assemblyscript&logoColor=white)]()
</div>

## 🧪 About RadiumOS

RadiumOS is an experimental hobby operating system project created for learning and educational purposes. This is a work-in-progress OS kernel that demonstrates basic operating system concepts and low-level programming.

> **⚠️ Warning**: This is a hobby project with many limitations and bugs. It is NOT intended for production use or as a replacement for any existing operating system.

## 🎯 Project Goals

- Learn operating system development fundamentals
- Understand low-level hardware interaction
- Explore kernel programming concepts
- Practice systems programming in C
- Build a basic bootable kernel

## 🔧 Current Features

- [x] Basic bootloader
- [x] Simple kernel initialization
- [x] Basic memory management (limited)
- [x] Simple text output
- [x] Keyboard driver
- [ ] File system support
- [x] Process management
- [ ] Network stack (Not Yet)

## ⚠️ Known Issues & Limitations

- **Memory Management**: Very basic, prone to leaks and corruption
- **Hardware Support**: Limited to specific configurations
- **Stability**: Frequent crashes and undefined behavior
- **Security**: No security features implemented
- **Performance**: Not optimized, educational code only
- **Compatibility**: May not work on all hardware
- **Documentation**: Incomplete and sparse

## 🛠️ Building & Testing

### Prerequisites

<div align="center">

![C](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white)
![NASM](https://img.shields.io/badge/NASM-654FF0?style=for-the-badge&logo=assemblyscript&logoColor=white)
![QEMU](https://img.shields.io/badge/QEMU-FF6600?style=for-the-badge&logo=qemu&logoColor=white)

</div>

```bash
# Ubuntu/Debian
sudo apt install build-essential nasm qemu-system-x86 git

# Arch Linux
sudo pacman -S base-devel nasm qemu git

# macOS (with Homebrew)
brew install nasm qemu i686-elf-gcc