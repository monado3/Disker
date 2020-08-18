# Disker
---
## Requirements
- Poetry (Python package tool)
- gcc
- make

## Installation
1. `$ ./install.sh`
1. edit some macros of `src/*.c` & `src/*.h` adjusting to your environemnt

## Usage
1. `$ poetry shell`
1. `$ python main.py --read --write -abr`

**Warning: Write measuring could logically destroy the HDD. You should prepare a destructible device file.**

## Architecture
- main.py : broker of C executables & grapher
- grapher.py : tool for plotting
- src/ : C source codes for microbenchmark
- bin/ : binary files complied from src/
- exp/ : **Not git tracked.** keeps log and figures of microbenchmark
- doc/ : Reports on my envrironment using graphs & logs genereated by Disker (as a sample output)

## Knowledge
- You have to make not only a buffer address but also lseek address 512 Bytes aligned when O_DIRECT is on.
