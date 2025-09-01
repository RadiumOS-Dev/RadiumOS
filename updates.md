# RadiumOS Updates & Changelog

<div align="center">
  <img src="https://raw.githubusercontent.com/RadiumOS-Dev/RadiumOS/main/images/radiumOS.png" alt="RadiumOS Logo" width="100"/>
  
  ![Status](https://img.shields.io/badge/status-experimental-red.svg)
  ![Last Update](https://img.shields.io/badge/last%20update-December%202024-blue.svg)
</div>

## 📋 Development Log

This document tracks the progress, updates, and changes made to RadiumOS. As a hobby project, updates may be irregular and features may be incomplete.

---

## 🚀 Version 0.0.2 - Current Update September 1 2025
*finished*

### 🔄 Currently Done
- `src/keyboard/keyboard.h` Updated : Keyboard interrupts 
- `src/keyboard/keyboard.h` Updated : ArrowKey Cursor (left/right)
- `src/mpop/mpop` Updated : Uses keyboard interrupts now
- `src/login/login.h` Removed Due to No Need
- `src/vfs/*` Removed Due to No Need
- `src/commands/text.h` Added text editor (NO FILESYSTEM JUST INTERACTIVE MPOP LANGUAGE)
### 🐛 Known Issues Being Addressed
---
```
Issue: Duplicate Text
Trigger: More than one of text inside shell
Status: Under Investigation
Priority: Low
```

## 🚀 Version 0.0.1 - Current Update August 31st 2025
*Finished*

### 🔄 Currently Done
- `src/commands/wifi.h` Removed Due to No Need
- `src/wifi/wifi.h`   Removed Due to No Need
- `src/kernel/kernel.c`(Edited) Removed Wifi Headers/Wifi Command
- `updates.md` Created


### 🐛 Known Issues Being Addressed
```
Issue: Driver breaks when key is held for extended periods
Trigger: Holding any key longer than 3 lines of text output
Status: Under investigation
Priority: High
```
```
Issue: Exit command fails in tasking environment
Behavior: Tasking works with keyboard driver initially
Problem: exit command causes system failure
Suspected Cause: Interrupt disabling may break system state
Status: Debugging in progress
Priority: High
```

