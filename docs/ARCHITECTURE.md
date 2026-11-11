# Architecture

> Português: [pt-BR/ARQUITETURA.md](pt-BR/ARQUITETURA.md)

Two executables are built from a shared library of sources in `src/`:

| Binary | Entry point | Role |
|--------|-------------|------|
| `montador` | `montador_main.cpp` | Assembles one `.asm` into `.OBJ` |
| `ligador` | `ligador_main.cpp` | Calls `montador --modular` for each module, then links into `.LIG` |

```
src/
├── common.{hpp,cpp}     instruction table (14 opcodes), directive list, error types
├── lexer.{hpp,cpp}      line -> tokens, lexical errors
├── assembler.{hpp,cpp}  parsing, symbol table, semantic checks, code generation
├── objfile.{hpp,cpp}    object formats (plain / modular), file helpers
├── linker.{hpp,cpp}     relocation + external symbol resolution
├── montador_main.cpp    CLI of the assembler
└── ligador_main.cpp     CLI of the linker
```

## Assembly pipeline (two passes)

1. **Lexical analysis** (`lexer.cpp`) — each line is cut at `;` (comment) and tokenised
   (`Ident`, `Number`, `:`, `+`, `-`, `,`). Identifiers are upper-cased, which makes the language
   case-insensitive. Bad tokens (invalid character, identifier longer than 30 characters, token
   starting with a digit such as `1ABC` or `0x10`) become **LEXICO** errors.
2. **Pass 1 — syntax + symbols** (`Assembler::parseLine`) — recognises `label:`, instructions and
   directives, checks operand counts, tracks the current section, and records every label in the
   symbol table with its *section-relative* address. A label alone on a line is kept as *pending* and
   attached to the next statement. `EQU`/`IF` are resolved here: a false `IF` discards the next line.
3. **Pass 2 — semantics + code** (`Assembler::generate`) — the text size is now known, so data labels
   receive `address + textSize`. Operands are resolved (`LABEL + N` supported), semantic checks run and
   the words are emitted: **program first, then CONST/SPACE data**, regardless of the order of the sections in the file.

The whole file is always analysed; errors are collected, sorted by line and printed. **No `.OBJ` is written when there is any error.**

## Diagnostics

| Class | Examples reported |
|-------|-------------------|
| **LEXICO** | invalid character; identifier > 30 chars; token starting with a digit (`1ABC`, hex `0x10`); number out of range |
| **SINTATICO** | wrong operand count (instructions, `SPACE`, `CONST`, `PUBLIC`…); unknown instruction/directive; comma between operands; two labels on one statement; invalid `SECTION` argument; dangling label |
| **SEMANTICO** | undeclared label; duplicated label/section; `TEXT` section missing; instruction in `DATA` / `CONST`,`SPACE` in `TEXT`; `EQU` after a section; `IF` outside `TEXT`; jump to a data label; code label used as data operand; writing to a `CONST`; division by a constant `0`; `BEGIN` without `END` (and vice versa); `PUBLIC` symbol not defined |

A line that already has a lexical error is not analysed further for syntax (avoids cascades), and
identical messages on the same line are reported once.

## Interpretation decisions

The assignment leaves a few points open. These are the choices made (all covered by `tests/`):

| Topic | Decision |
|-------|----------|
| Section keyword | Both `SECAO` (assignment text) and `SECTION` (Moodle samples) are accepted |
| `COPY A, B` | Syntax error (SINTATICO): comma is not part of the language, test files are never "fixed" |
| Hexadecimal `CONST` | Rejected as LEXICO — the assignment says *decimal only* |
| Output bytes | Every word followed by one blank, no trailing newline — byte-identical to the reference `bin.obj` |
| `END` | Does not stop parsing (the course sample puts `SECTION DATA` after `END`); `BEGIN`/`END` must be paired |
| `NAME: BEGIN` / `BEGIN: NAME` | Both accepted (the course sample uses the second form) |
| `NAME: EXTERN` / `EXTERN NAME` | Both accepted |
| Two labels before one statement | Error, also when split across lines |
| False `IF` | Discards the next non-empty line |
| Extra semantic checks | const write, division by constant zero, jump/operand kind mismatch — classic test-suite cases beyond the 6 listed |
| `.LIG` name | Named after the first module (`prog1.asm` → `prog1.LIG`) |

## Linker

`ligador` runs `montador --modular` on every file (found next to the `ligador` binary, then `./`, then `PATH`),
parses the generated modular objects ([OBJECT_FORMAT.md](OBJECT_FORMAT.md)), and:

1. computes each module's **base** (sum of the sizes of the previous modules);
2. builds the **global symbol table** from `PUBLIC` definitions (duplicates are errors);
3. adds the base to every word flagged in the **relocation** row;
4. for every **use** of an `EXTERN` symbol, adds the symbol's global address to the word;
5. writes the single-line `.LIG`.

Unresolved externals and duplicated publics abort the link with `[ERRO LIGACAO]` messages.
