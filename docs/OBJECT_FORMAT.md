# Object formats

> Português: [pt-BR/FORMATO_OBJETO.md](pt-BR/FORMATO_OBJETO.md)

## 1. Plain object — `montador prog.asm` → `prog.OBJ`

One text line. Words separated by blanks, program first, then `CONST`/`SPACE` data (`SPACE` = `0`).
Each word is followed by one blank and there is no final newline (the layout of the reference `bin.obj`).

```
12 29 10 29 4 28 11 30 3 28 11 31 10 29 2 31 11 31 13 31 9 30 29 10 29 7 4 14 2 0 0 0 
```

## 2. Modular object — `montador --modular prog.asm` (used by `ligador`)

The assignment leaves this format free. Layout (follows the example `LIG.OBJ.txt` from the course):

```
D, <name> <address>      one line per PUBLIC symbol (address inside the module)
U, <name> <address>      one line per use of an EXTERN symbol (address of the operand word)
R, <b0> <b1> ... <bn>    one relocation bit per word: 1 = address that depends on the load base
<w0> <w1> ... <wn>       the code line (same as the plain object)
```

Example — `examples/modulo1.asm` (11 words):

```
D, L2 6
D, OLD_DATA 8
D, NEW_DATA 9
D, TMP_DATA 10
U, MOD2 5
R, 0 1 0 1 0 0 0 0 0 0 0
12 8 10 8 5 0 14 2 0 0 0
```

Conventions:

* A word that refers to a **local** label holds its module-relative address and has `R = 1`.
* A word that refers to an **external** symbol holds only the addend (`0`, or `N` in `X + N`), has `R = 0`, and appears in a `U` line.
* Opcodes and `CONST`/`SPACE` data have `R = 0`.

## 3. Linked executable — `ligador a.asm b.asm c.asm` → `a.LIG`

Same format as the plain object. For each module `m` with base `B(m)` (sum of the sizes of the previous modules):

```
final[w] = code[w] + B(m)                      if R[w] = 1
final[w] = code[w] + address(symbol)           for every U entry (symbol, w)
address(symbol) = B(defining module) + D address
```

Modules are laid out in command-line order, so module 1 starts at address 0 — where the simulator starts executing.
Each module keeps its own data right after its own code.

Example (`examples/modulo1.asm` + `examples/modulo2.asm`, bases 0 and 11):

```
12 8 10 8 5 11 14 2 0 0 0 | 4 36 11 9 3 36 11 10 10 8 2 10 11 10 13 10 9 9 8 10 8 7 11 5 6 2
```
