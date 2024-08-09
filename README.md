# Tiny brainfuck compiler

## Functionality

- Requires an x86 machine, works on linux but requires a tweak for it to compile for windows
- Requires `nasm`, `ld`, and `rm` to exist and be executable
- Creates files `/tmp/_bfc.asm` and `/tmp/_bfc.o` during compilation
- Likely unsafe to use as its made to be compact (not safe)

## How to use

- Compile using either autoc (my tool) or `cc -o bf ./bf.c`
- Run `bf <input> <output>`
- Enjoy!
