# nes-emulator
An emulator to play [NES games](https://en.wikipedia.org/wiki/List_of_Nintendo_Entertainment_System_games). No third-party library/source code used. 100% C++ code.

## Features
* Robust, Safe, Fast
* Bug-free CPU emulation
* Scanline-based rendering
* Video output using D3D9
* Custom log for debug (Disassembly, CPU state, PPU state)
* Easy to port to other OS and platforms

## Compatibility List
* Super Mario Bros.
* Super Mario Bros. 3
* Adventure Island.
* Legend of Zelda
* Mega Man 2
* Many many other roms that use mapper 0, 1, 2, 3 or 4 (Theoretical 60% of roms)

## Controls
* Button A: X
* Button B: Z
* Left/Right/Up/Down: Arrow Keys
* Select: Shift
* Start: Enter
* Load State: L
* Save State: S
* Reset: Esc
* Quit: Ctrl+Esc (Alt+F4 in DX9 mode)

## Known limitation
* APU is not implemented, so there's no sound while playing.