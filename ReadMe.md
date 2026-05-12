## Discription
This repository documents my journey of rebuilding the FreeRTOS kernel from a blank file. Instead of simply calling existing APIs, I am hand-coding every core component—from basic data structures to the task scheduler—to master the art of RTOS architecture and bare-metal embedded programming.

### Tech Stack
**Target Hardware**: ARM Cortex-M Series (STM32)
**Toolchain**: Keil uVision 5 (ARM Compiler 5/6)
**Language**: C (Strictly following the FreeRTOS Hungarian naming convention)
**Debugging**: Keil Simulator / ST-Link / Register-level pointer tracking

### Repo Structure
```
.
├── FreeRTOS_Source/    # Hand-coded kernel files (list.c, tasks.c, etc.)
├── Project/            # Keil uVision project files
├── Docs/               # Diagrams and logic flows for pointer jumps
└── README.md           # Main landing page
```