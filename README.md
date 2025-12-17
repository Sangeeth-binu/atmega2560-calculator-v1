# atmega2560-calculator-v1
Bare metal C implementation of a keypad driven calculator on ATmega2560. Focus: register level I/O, keypad scanning, blocking design.

# AVR Calculator (ATmega2560)

Bare metal calculator implemented in C using direct register access.

## Hardware
- MCU: ATmega2560
- Keypad: 4×4 matrix
- Seven Segment

## Design Goals
- No Arduino libraries
- Registerlevel GPIO control
- keypad scanning
- Clear separation between scan, decode, and action

## Status
Version 1 — keypad scan + key detection in progress.
