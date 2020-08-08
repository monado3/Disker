# Disker
---
## Requirements
- Poetry (Python package tool)
- gcc
- make

## Installation
1. `$ ./install.sh`
1. Edit some macros of `src/*.c` adjusting to your environemnt

## Usage
1. `$ poetry shell`
1. `$ python main.py -abr`

## Architecture
- main.py : broker of C & grapher
- grapher.py : tool for plotting
- src/ : C source codes for microbenchmark
- bin/ : Executables files build from src/
- exp/ : **Not git tracked.** keeps log and figures of microbenchmark

## Knowledge
- You have to make lseek address 512 Bytes aligned when O_DIRECT is on.
