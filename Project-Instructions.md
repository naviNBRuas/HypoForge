# Practical Assignment 1 — Basic Software (Software Básico): Project Instructions

> Organised English translation of the original assignment (`docs/assignment/trabalho1_sb1.pdf`, in Portuguese).
> Portuguese version: [Projeto_Instrucoes.md](Projeto_Instrucoes.md).

## 1. Introduction

Implement, in **C/C++**, a method that translates a simple assembly language into an object-code representation. The translator is an **assembler** for the hypothetical language presented in class.

## 2. Goal

Consolidate how a translation process works (two-pass and single-pass algorithms), the lexical, syntactic and semantic analysis stages, and the object-code generation stage.

## 3. Specification

### 3.1 Assembler

- The language has **14 instructions**.
- Programs are split into a **code section and a data section**. You may choose **one or two passes**.
- Use the instruction table from class (mnemonic, operand count, opcode, size in words).
- The assembler accepts the **SPACE** (takes **one** argument) and **CONST** directives.
- Identifiers (variables and labels) are limited to **30 characters** and follow C rules (letters, digits, `_`, not starting with a digit).
- Sections are marked with directives; the data section may come **before or after** the text section.

The assembler must:

- Be **case-insensitive** (instructions, directives and labels).
- Ignore tabs, line breaks and unnecessary blanks anywhere.
- Accept `CONST` with positive and negative numbers (**decimal only**).
- Accept comments introduced by `;` anywhere.
- Use `COPY` with a **blank, no comma**, between operands (`COPY A B`).
- Write array labels **without a comma**, with blanks (`ADD X + 2`).
- Allow a label on its own line, with the statement continuing on the next line.

The program must be called **`montador`** and receive the input file (with extension) as a command-line argument (`./montador myprogram.asm`). It writes a file **with the same name and the extension changed to `.OBJ`**. *Following this behaviour exactly is mandatory; otherwise the work automatically gets ZERO.*

The output is **plain text**, a **single line**, blank-separated, with no addresses and no section separation; `CONST`/`SPACE` data comes **after** the program. `SPACE` is written as `00` or `0` (never `XX`).

### Errors to detect

The whole program is always assembled, printing the **line(s)** of the original file and the **error type** (**LEXICO**, **SINTATICO** or **SEMANTICO**):

1. missing declarations and labels;
2. repeated declarations and labels;
3. invalid directives and instructions;
4. missing text section;
5. lexical errors;
6. instructions/directives with the wrong number of operands.

Grading uses many programs besides those on Moodle, and test files are **not** modified to fit your program (e.g. `COPY` with a comma will not be fixed).

### 3.2 EQU and IF

`EQU` is always declared **before any section**; `IF` only **inside the text section**.

### 3.3 Linker

Usage: `./ligador prog1.asm prog2.asm prog3.asm` (at most 3 programs).

- The linker **calls the assembler internally** to create each object file.
- The `.asm` files use **`BEGIN`, `END`, `PUBLIC` and `EXTERN`**.
- The intermediate object format is **free**; the linker creates the **`.LIG`** executable in the same single-line format.

## Instruction table (hypothetical machine)

| Mnemonic | Operands | Opcode | Size (words) |
|----------|:--------:|:------:|:------------:|
| ADD | 1 | 1 | 2 |
| SUB | 1 | 2 | 2 |
| MULT | 1 | 3 | 2 |
| DIV | 1 | 4 | 2 |
| JMP | 1 | 5 | 2 |
| JMPN | 1 | 6 | 2 |
| JMPP | 1 | 7 | 2 |
| JMPZ | 1 | 8 | 2 |
| COPY | 2 | 9 | 3 |
| LOAD | 1 | 10 | 2 |
| STORE | 1 | 11 | 2 |
| INPUT | 1 | 12 | 2 |
| OUTPUT | 1 | 13 | 2 |
| STOP | 0 | 14 | 1 |

> The table was verified against the reference object file shipped with the assignment (`bin.obj`); see `tests/valid/bin.*`.
