# Atari 2600 Game Console Emulator
This is an emulator for the famous 1977 videogame console Atari 2600 which sold over 30 million copies to this day. It allows you to play the games for Atari 2600 on your computer.

![Atari 2600 console photo](https://upload.wikimedia.org/wikipedia/commons/thumb/0/02/Atari-2600-Wood-4Sw-Set.png/960px-Atari-2600-Wood-4Sw-Set.png)

Screenshots from famous games for Atari 2600:
![A screenshot from Berzerk](https://atariage.com/2600/screenshots/s_Berzerk_2.png)
![A screenshot from Pitfall](https://atariage.com/2600/screenshots/s_Pitfall_1.png)
![A screenshot from Solaris](https://atariage.com/2600/screenshots/s_Solaris_2.png)

## Dependencies
The emulator uses C language and the only dependency is the Raylib graphics library. Make sure you have it installed in your system. The emulator is tested on Linux, however it should also work on Windows, Mac OSX and other systems.

## Features
- Supports 3 input modes: Joysticks, Paddles, Keypad/Numpads.
- Supports NTSC, PAL, SECAM and MONOCHROME video standards.
- Supports the following mappers/bankswitching methods: 2K, 4K, CV, F8, F6, F4, FE, E0, FA, E7, F0.
- CPU emulates 248 out of 256 instruction types correctly.
- CPU is cycle-accurate and emulates ghost read/writes correctly.
- TIA and RIOT chips are almost cycle-accurate.
- Supports audio.
- Most examples in https://8bitworkshop.com/v3.12.0/?platform=vcs work properly.
- Writes to certain registers like HMOVE are delayed by a few clocks properly.
- Written in 9k lines of C. Only dependency is Raylib.
- A simple debugger is included.

## Limitations
- CPU doesn't emulate the following instructions: 6B, 8B, AB, 93, 9F, 9C, 9E, 9B. These are known to be unstable.
- HMOVE goes to the final value directly, doesn't transition for 24 cycles.
- HMOVE comb is left out intentionally.
- Audio is mono.
- VSYNC isn't required to be exactly 3 cycles.
- Raylib might not register 3 or more keys pressed at the same time.
- The games are played entirely from keyboard. External gamepads are not supported.

## Compilation
To compile, just type "make". It assumes you have GCC, make installed. The dependency Raylib needs to be installed beforehand separately. The code uses POSIX APIs common to Linux, Mac OSX, BSD systems. It wasn't tested on Windows. Most likely it will need a bit of tweaking to make it work there. It uses command line to pass the arguments. It takes less than 10 seconds to compile.

## Resources
- Main document used as reference: https://problemkaputt.de/2k6specs.htm
- "Stella Programmer's Guide" book
- A collection of 500+ games from https://atariage.com
- For testing assembly code, and for examples: https://8bitworkshop.com/v3.12.0/?platform=vcs
- The source code and the debugger of the reference emulator: https://github.com/stella-emu/stella
- HMOVE details: https://www.biglist.com/lists/stella/archives/199804/msg00198.html
- For illegal instructions: https://www.masswerk.at/nowgobang/2021/6502-illegal-opcodes
- https://www.nesdev.org/6502_cpu.txt
- https://www.nesdev.org/extra_instructions.txt
- https://www.nesdev.org/undocumented_opcodes.txt
- To test sounds: https://forums.atariage.com/topic/123441-tone-toy-2008-find-new-sound-effects-for-your-games/
- RIOT datasheet, needed for timer: http://retro.hansotten.nl//uploads/riot653x/ds_6532.pdf
- To test bankswitching: https://forums.atariage.com/topic/141764-bankswitching-demostemplates/page/4/

## Note
This is a hobby project. It doesn't aim to be better or more accurate than established emulators: Stella, Javatari, Gopher2600. Use those instead.
