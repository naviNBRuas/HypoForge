# Trabalho Prático 1 — Software Básico (Instruções do projeto)

> Transcrição organizada do enunciado original (`docs/assignment/trabalho1_sb1.pdf`).
> Versão em inglês: [Project-Instructions.md](Project-Instructions.md).

## Universidade

Universidade de Brasília  
Departamento de Ciência da Computação  
CIC0104 — Software Básico, turma T02

## 1. Introdução

O trabalho consiste em implementar em **C/C++** um método de tradução de uma linguagem de montagem simples para uma representação de código objeto. O tradutor a ser implementado será um **Assembler** da linguagem hipotética vista em sala de aula.

## 2. Objetivo

Fixar o funcionamento de um processo de tradução (algoritmo de 2 passagens e passagem única), as etapas de análise léxica, sintática e semântica e a etapa de geração de código objeto.

## 3. Especificação

### 3.1 Montador

- A linguagem de montagem é a linguagem simbólica hipotética apresentada em sala, com **14 instruções**.
- Os programas são divididos em **seção de código e seção de dados**. Pode-se escolher entre **uma ou duas passagens**.
- Para cada instrução usa-se a tabela vista em sala (mnemônico, nº de operandos, opcode, tamanho em palavras).
- O montador aceita as diretivas **SPACE** (recebe **um** argumento) e **CONST**.
- Identificadores de variáveis e rótulos têm no máximo **30 caracteres**; seguem as regras de C (letras, dígitos e `_`, sem começar por dígito).
- As seções são marcadas por diretivas; a seção de dados pode vir **antes ou depois** da seção de texto:

```
SECAO DATA
N1: SPACE
N2: CONST -48
N4: SPACE

SECAO TEXT
ROT: INPUT N1
COPY N1 N4 ;comentario
COPY N2 N3
COPY N3 N3
OUTPUT N3
STOP
```

O montador deve:

- **Não diferenciar maiúsculas de minúsculas** (instruções, diretivas e rótulos).
- Desconsiderar tabulações, quebras de linha e espaços desnecessários em qualquer lugar.
- Aceitar `CONST` com números positivos e negativos (**somente decimal**).
- Aceitar comentários iniciados por `;` em qualquer lugar.
- Usar `COPY` com **espaço, sem vírgula**, entre os operandos (`COPY A B`).
- Aceitar rótulo de array **sem vírgula**, com espaço (`ADD X + 2`).
- Permitir criar um rótulo, quebrar a linha e continuar depois (o rótulo equivale à linha seguinte).

O programa deve se chamar **`montador`** e receber o arquivo de entrada (com extensão) por argumento (`./montador myprogram.asm`). A saída é o arquivo **com o mesmo nome, trocando a extensão para `.OBJ`**. *Seguir exatamente esse funcionamento é obrigatório; caso contrário o trabalho recebe nota ZERO.*

O arquivo de saída está em **formato texto**, **uma única linha**, com espaço entre opcode e operandos. Para `SPACE`, grava-se `00` ou `0` (nunca `XX`). Não há separação de seções no objeto; as constantes e espaços (`CONST`/`SPACE`) vêm **depois** do programa. Não se indicam endereços.

### Erros a identificar

O programa inteiro deve ser montado, mostrando na tela a(s) **linha(s)** do arquivo original e o **tipo do erro** (**LEXICO**, **SINTATICO** ou **SEMANTICO**):

1. declarações e rótulos ausentes;
2. declarações e rótulos repetidos;
3. diretivas e instruções inválidas;
4. seção de texto ausente;
5. erros léxicos;
6. instruções e diretivas com a quantidade de operandos errada.

Na correção serão usados vários programas além dos disponibilizados no Moodle. Os arquivos de teste **não** serão modificados para se adaptar ao programa (por exemplo, `COPY` com vírgula não será corrigido).

### 3.2 EQU e IF

O montador deve aceitar `EQU` e `IF` como visto em sala. `EQU` é sempre declarado **antes de qualquer seção**; `IF` somente **dentro da seção de texto**.

### 3.3 Ligador

Uso: `./ligador prog1.asm prog2.asm prog3.asm` (no máximo 3 programas).

- O ligador **chama internamente o montador** para gerar o objeto de cada `.asm`.
- Os `.asm` usam as diretivas **`BEGIN`, `END`, `PUBLIC` e `EXTERN`**.
- O formato dos objetos intermediários é **livre**; o ligador cria o arquivo **`.LIG`** (executável) no mesmo formato de uma única linha, com espaço entre opcode e operandos, `SPACE` como `0`/`00`, constantes e espaços depois do programa, sem endereços.

## Tabela de instruções (máquina hipotética)

| Mnemônico | Operandos | Opcode | Tamanho (palavras) |
|-----------|:---------:|:------:|:------------------:|
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

> A tabela acima foi conferida contra o objeto de referência fornecido (`bin.obj`); ver `tests/valid/bin.*`.
