# HypoForge

English | [Português](README_PT_BR.md)

HypoForge is a two-pass assembler (`montador`) and linker (`ligador`) for the 14-instruction hypothetical machine, built for Practical Assignment 1 of CIC0104 — Software Básico (Basic Software), class T02, at the University of Brasília. It demonstrates:

- two-pass assembly with a symbol table
- lexical, syntactic and semantic analysis with per-line error reports
- single-line text object generation (`.OBJ`)
- the `SPACE`, `CONST`, `EQU` and `IF` directives and `LABEL + N` addressing
- linking of up to three modules (`BEGIN`, `END`, `PUBLIC`, `EXTERN`) with relocation
- end-to-end runs on the course simulator

## Project goals covered

- `./montador prog.asm` writes `prog.OBJ`: same name, extension changed to `.OBJ`
- plain-text output on a single line, program first and `CONST`/`SPACE` data after it (`SPACE` = `0`)
- case-insensitive source; tabs, blanks, blank lines and `;` comments ignored anywhere
- `CONST` with positive and negative decimal numbers
- `COPY A B` without a comma, `ADD X + 2` for arrays, and a label alone on a line labelling the next one
- `DATA` section before or after `TEXT`; both `SECAO` and `SECTION` accepted
- the whole file is always analysed and every error is printed with its line and type (`LEXICO`, `SINTATICO` or `SEMANTICO`)
- detection of the six required error groups: missing and repeated labels, invalid directives and instructions, missing `TEXT` section, lexical errors, and wrong operand counts
- `EQU` before any section and `IF` inside `TEXT`
- `./ligador prog1.asm prog2.asm prog3.asm` calls the assembler internally and writes the `.LIG` executable

## Tech stack

- C++17 (also builds as C++14), standard library only
- GNU Make
- Bash test runner
- Python 3 for the stress tests (optional)
- LaTeX (`IEEEtran`) for the report (optional)

## Quick start

1. Build both tools.
1. Assemble the course example.
1. Link the two-module example.
1. Run the result on the course simulator.

```bash
make
./montador examples/bin.asm                           # -> examples/bin.OBJ
./ligador examples/modulo1.asm examples/modulo2.asm   # -> examples/modulo1.LIG
echo 13 | tools/simulador/simulador examples/modulo1.LIG
# prints 1 0 1 1 (binary digits of 13, least significant first)
```

## Reproducibility (step-by-step)

Prerequisites:

- a C++14-or-newer compiler (`g++`, MinGW or clang)
- `make` and `bash` (optional on Windows, see step 2)
- Python 3 (only for `make stress`)
- TeX Live with `latexmk` and `IEEEtran` (only for the report)

1. Clone the repository and build:

```bash
git clone https://github.com/naviNBRuas/HypoForge.git
cd HypoForge
make
```

1. Without `make` (e.g. Windows with MinGW), build from the repository root. Keep `montador` and `ligador` in the same directory, because the linker calls the assembler:

```bash
g++ -std=c++17 -O2 -o montador src/montador_main.cpp src/assembler.cpp src/lexer.cpp src/common.cpp src/objfile.cpp
g++ -std=c++17 -O2 -o ligador src/ligador_main.cpp src/linker.cpp src/objfile.cpp src/assembler.cpp src/lexer.cpp src/common.cpp
```

1. Run the whole test suite (81 checks):

```bash
make test
```

1. Assemble the course example and compare it with the reference object:

```bash
./montador examples/bin.asm && cmp examples/bin.OBJ examples/bin.expected.OBJ
```

1. Provoke errors:

```bash
./montador tests/invalid/spec_example.asm
```

1. Run the stress tests (fuzzing and differential testing):

```bash
make stress
```

1. Build the reports (PDF):

```bash
make report
# report/main.pdf (English) and report/main_pt_br.pdf (Portuguese)
```

The simulator in `tools/simulador/` is the one provided by the course (`simulador` for Linux, `simulador.exe` for Windows). If it cannot run on your machine, `make test` marks the simulator cases as skipped.

## Commands

- `make`
- `make test`
- `make stress`
- `make report`
- `make clean`

## Usage and output

| Tool | Input | Output |
|------|-------|--------|
| `./montador prog.asm` | assembly file | `prog.OBJ`: one line of blank-separated words |
| `./ligador a.asm b.asm c.asm` | 1 to 3 modules | `a.OBJ`, `b.OBJ`, `c.OBJ` (modular objects) and `a.LIG` (executable) |

On errors the assembler prints every problem with its line and type, then the source line, and writes no `.OBJ`:

```text
$ ./montador tests/invalid/spec_example.asm
[ERRO SEMANTICO] linha 11: rotulo 'N3' nao declarado
    11 | COPY N2 N3
[ERRO SEMANTICO] linha 12: rotulo 'N3' nao declarado
    12 | COPY N3 N3
...
```

The console messages are in Portuguese because the assignment names the error types `LEXICO`, `SINTATICO` and `SEMANTICO`.

## Testing

`make test` runs 81 checks:

| Group | Cases | What is compared |
|-------|------:|------------------|
| `tests/valid/` | 25 | `.OBJ` byte by byte against hand-computed objects, including the course `bin.asm` |
| `tests/invalid/` | 28 | every reported `line TYPE` pair, and that no `.OBJ` is written |
| `tests/link/` | 13 | `.LIG` byte by byte, simulator output and link-error messages |
| `tests/sim/` | 6 | output of assembled programs on the course simulator |
| command line | 9 | file names with dots, blanks and `.ASM`, missing files, argument counts, stale outputs |

The cases include inputs a grader is likely to try:

- a label alone on a line, followed by comments and blank lines
- identifiers of exactly 30 and 31 characters
- `X+1` without blanks, and `COPY A,B` with a comma
- hexadecimal, fractional, `-0` and out-of-range constants
- accented characters, reserved words as labels, and labels differing only in case
- nested `IF`, and an `IF` that drops a labelled line
- a UTF-8 BOM, and CRLF or CR-only line endings
- `LABEL + N` past the end of a `SPACE`, and every instruction once

`make stress` adds two tools:

- `scripts/differential.py` generates 3,000 random valid programs, computes their objects independently and requires byte-for-byte equality
- `scripts/fuzz.py` feeds 6,000 random or mutated programs and 600 random links to the tools and fails on any crash

Both were also run with AddressSanitizer and UBSan.

## Repository layout

- `src/` — assembler, linker and shared code
- `tests/` — `valid/`, `invalid/`, `link/` and `sim/` cases with their expected results
- `examples/` — course samples (`bin.asm` and the two-module example)
- `docs/` — [architecture](docs/ARCHITECTURE.md), [object formats](docs/OBJECT_FORMAT.md) (Portuguese in `docs/pt-BR/`) and the original assignment PDF
- `report/` — IEEE LaTeX report in English (`main.tex`) and Portuguese (`main_pt_br.tex`)
- `scripts/` — test runner, fuzzer and differential tester
- `tools/simulador/` — course simulator (Linux and Windows)
- `Project-Instructions.md` / `Projeto_Instrucoes.md` — the assignment, organised in English and Portuguese

## Report

The report follows the IEEE conference format with the UnB template. The English version is `report/main.tex` and its Portuguese translation, submitted to the course, is `report/main_pt_br.tex`. Compiled PDFs are build outputs and are not versioned; run `make report` to produce them in the same folder.

## License

This repository is intended for academic use in the Software Básico assignment.

---
Files of this project are provided under the terms of the included `LICENSE` file (MIT by default). See `LICENSE` for details.
