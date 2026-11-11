# HypoForge — Assembler & Linker for a Hypothetical Machine

English | [Português](README_PT_BR.md)

**HypoForge** is a two-pass **assembler** (`montador`) and **linker** (`ligador`) for the hypothetical 14-instruction machine of the *Software Básico* course
(University of Brasília, UnB), written in C++17 with no external dependencies.

- `montador` — translates `.asm` into a single-line text object (`.OBJ`), reporting **lexical, syntactic and semantic** errors with the offending lines
- `ligador` — links up to three modules (`BEGIN`/`END`/`PUBLIC`/`EXTERN`) into an executable `.LIG`
- full support for `SPACE n`, `CONST`, `LABEL + N` addressing, `EQU`/`IF`, comments, case-insensitivity and labels split across lines
- test-suite with hand-computed expectations, plus end-to-end runs on the course simulator

## Quick start

```
make                                   # builds ./montador and ./ligador
./montador examples/bin.asm            # -> examples/bin.OBJ
./ligador examples/modulo1.asm examples/modulo2.asm   # -> examples/modulo1.LIG
echo 13 | tools/simulador/simulador examples/modulo1.LIG   # prints 1 0 1 1 (binary digits, LSB first)
```

Requirements: a C++17 compiler (`g++` ≥ 8, MinGW works) and `make`. LaTeX (`pdflatex`, `latexmk`, IEEEtran) is only needed for the report.

## Reproducibility (step-by-step)

1. Build: `make`
2. Run the whole suite: `make test` (assembler valid/invalid programs, linker cases, simulator runs)
3. Assemble the provided sample and compare with the reference object:
   `./montador examples/bin.asm && cmp examples/bin.OBJ examples/bin.expected.OBJ`
4. Provoke errors: `./montador tests/invalid/semantic.asm`
5. Build the report (PDF): `make report` → `report/main.pdf`

The simulator in `tools/simulador/` is the course-provided one (`simulador` for Linux, `simulador.exe` for Windows);
if it cannot run on your machine, `make test` marks those cases as *skipped*.

## Behaviour summary

| Tool | Input | Output |
|------|-------|--------|
| `./montador prog.asm` | assembly file | `prog.OBJ` — one line, words separated by blanks, program first then `CONST`/`SPACE` data |
| `./ligador a.asm b.asm c.asm` | 1–3 modules | `a.OBJ`, `b.OBJ`, `c.OBJ` (modular objects) and `a.LIG` (executable) |

On errors the assembler analyses the **whole** file and prints, for each problem,
`[ERRO <LEXICO|SINTATICO|SEMANTICO>] linha N: message` plus the source line; no `.OBJ` is written.

```
$ ./montador tests/invalid/spec_example.asm
[ERRO SEMANTICO] linha 11: rotulo 'N3' nao declarado
    11 | COPY N2 N3
...
```

## Commands

- `make` — build
- `make test` — run `scripts/run_tests.sh`
- `make report` — build `report/main.pdf`
- `make clean` — remove binaries and objects

## Repository layout

- `src/` — assembler, linker and shared code
- `tests/` — `valid/`, `invalid/`, `link/`, `sim/` cases with their expected results
- `examples/` — course samples (`bin.asm`, two-module example)
- `docs/` — [architecture](docs/ARCHITECTURE.md), [object formats](docs/OBJECT_FORMAT.md) (Portuguese in `docs/pt-BR/`) and the original assignment PDF
- `report/` — IEEE LaTeX report in Portuguese (UnB template)
- `scripts/` — test runner
- `tools/simulador/` — course simulator (Linux and Windows)
- `Projeto_Instrucoes.md` / `Project-Instructions.md` — the assignment, organised in Portuguese and English

## Report

The report is written in Portuguese in IEEE format and lives in `report/main.tex`
(fill in the author macros at the top of the file before building).

## License

MIT — see [LICENSE](LICENSE). Intended for academic use.
