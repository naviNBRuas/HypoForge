# HypoForge

[English](README.md) | Português

O HypoForge é um montador (`montador`) de duas passagens e um ligador (`ligador`) para a máquina hipotética de 14 instruções, desenvolvidos para o Trabalho Prático 1 da disciplina CIC0104 — Software Básico, turma T02, na Universidade de Brasília. O projeto demonstra:

- montagem em duas passagens com tabela de símbolos
- análise léxica, sintática e semântica com erros reportados por linha
- geração de código objeto em texto, em uma única linha (`.OBJ`)
- as diretivas `SPACE`, `CONST`, `EQU` e `IF` e o endereçamento `ROTULO + N`
- ligação de até três módulos (`BEGIN`, `END`, `PUBLIC`, `EXTERN`) com realocação
- execução ponta a ponta no simulador da disciplina

## Objetivos atendidos

- `./montador prog.asm` gera `prog.OBJ`: mesmo nome, extensão trocada por `.OBJ`
- saída em texto, em uma única linha, com o programa primeiro e os dados `CONST`/`SPACE` depois (`SPACE` = `0`)
- fonte insensível ao caso; tabulações, espaços, linhas vazias e comentários com `;` ignorados em qualquer lugar
- `CONST` com números decimais positivos e negativos
- `COPY A B` sem vírgula, `ADD X + 2` para vetores e rótulo sozinho na linha rotulando a linha seguinte
- seção `DATA` antes ou depois de `TEXT`; `SECAO` e `SECTION` são aceitas
- o arquivo inteiro é sempre analisado e cada erro é exibido com a linha e o tipo (`LEXICO`, `SINTATICO` ou `SEMANTICO`)
- detecção dos seis grupos de erro exigidos: rótulos ausentes e repetidos, diretivas e instruções inválidas, seção `TEXT` ausente, erros léxicos e quantidade errada de operandos
- `EQU` antes de qualquer seção e `IF` dentro de `TEXT`
- `./ligador prog1.asm prog2.asm prog3.asm` chama o montador internamente e gera o executável `.LIG`

## Tecnologias

- C++17 (também compila como C++14), apenas biblioteca padrão
- GNU Make
- executor de testes em Bash
- Python 3 para os testes de estresse (opcional)
- LaTeX (`IEEEtran`) para o relatório (opcional)

## Execução rápida

1. Compile as duas ferramentas.
1. Monte o exemplo da disciplina.
1. Ligue o exemplo de dois módulos.
1. Execute o resultado no simulador da disciplina.

```bash
make
./montador examples/bin.asm                           # -> examples/bin.OBJ
./ligador examples/modulo1.asm examples/modulo2.asm   # -> examples/modulo1.LIG
echo 13 | tools/simulador/simulador examples/modulo1.LIG
# imprime 1 0 1 1 (dígitos binários de 13, do menos significativo ao mais)
```

## Reprodutibilidade (passo a passo)

Pré-requisitos:

- compilador C++14 ou mais recente (`g++`, MinGW ou clang)
- `make` e `bash` (opcionais no Windows, veja o passo 2)
- Python 3 (apenas para `make stress`)
- TeX Live com `latexmk` e `IEEEtran` (apenas para o relatório)

1. Clone o repositório e compile:

```bash
git clone https://github.com/naviNBRuas/HypoForge.git
cd HypoForge
make
```

1. Sem `make` (por exemplo, no Windows com MinGW), compile a partir da raiz do repositório. Mantenha `montador` e `ligador` no mesmo diretório, pois o ligador chama o montador:

```bash
g++ -std=c++17 -O2 -o montador src/montador_main.cpp src/assembler.cpp src/lexer.cpp src/common.cpp src/objfile.cpp
g++ -std=c++17 -O2 -o ligador src/ligador_main.cpp src/linker.cpp src/objfile.cpp src/assembler.cpp src/lexer.cpp src/common.cpp
```

1. Execute toda a suíte de testes (81 verificações):

```bash
make test
```

1. Monte o exemplo da disciplina e compare com o objeto de referência:

```bash
./montador examples/bin.asm && cmp examples/bin.OBJ examples/bin.expected.OBJ
```

1. Provoque erros:

```bash
./montador tests/invalid/spec_example.asm
```

1. Execute os testes de estresse (fuzzing e teste diferencial):

```bash
make stress
```

1. Compile os relatórios (PDF):

```bash
make report
# report/main_pt_br.pdf (português) e report/main.pdf (inglês)
```

O simulador em `tools/simulador/` é o fornecido pela disciplina (`simulador` para Linux, `simulador.exe` para Windows). Se ele não executar na sua máquina, `make test` marca os casos do simulador como *skipped*.

## Comandos

- `make`
- `make test`
- `make stress`
- `make report`
- `make clean`

## Uso e saída

| Ferramenta | Entrada | Saída |
|------------|---------|-------|
| `./montador prog.asm` | arquivo assembly | `prog.OBJ`: uma linha de palavras separadas por espaço |
| `./ligador a.asm b.asm c.asm` | 1 a 3 módulos | `a.OBJ`, `b.OBJ`, `c.OBJ` (objetos modulares) e `a.LIG` (executável) |

Havendo erros, o montador exibe cada problema com a linha e o tipo, seguido da linha do fonte, e não gera `.OBJ`:

```text
$ ./montador tests/invalid/spec_example.asm
[ERRO SEMANTICO] linha 11: rotulo 'N3' nao declarado
    11 | COPY N2 N3
[ERRO SEMANTICO] linha 12: rotulo 'N3' nao declarado
    12 | COPY N3 N3
...
```

## Testes

`make test` executa 81 verificações:

| Grupo | Casos | O que é comparado |
|-------|------:|-------------------|
| `tests/valid/` | 25 | `.OBJ` byte a byte com objetos calculados à mão, incluindo o `bin.asm` da disciplina |
| `tests/invalid/` | 28 | cada par `linha TIPO` reportado, e que nenhum `.OBJ` seja gerado |
| `tests/link/` | 13 | `.LIG` byte a byte, saída no simulador e mensagens de erro de ligação |
| `tests/sim/` | 6 | saída dos programas montados no simulador da disciplina |
| linha de comando | 9 | nomes com pontos, espaços e `.ASM`, arquivo inexistente, quantidade de argumentos, saídas antigas |

Os casos incluem entradas que um avaliador provavelmente testará:

- rótulo sozinho na linha, seguido de comentários e linhas vazias
- identificadores com exatamente 30 e 31 caracteres
- `X+1` sem espaços e `COPY A,B` com vírgula
- constantes hexadecimais, fracionárias, `-0` e fora do intervalo
- caracteres acentuados, palavras reservadas como rótulo e rótulos que diferem só no caso
- `IF` aninhado e `IF` que descarta uma linha com rótulo
- BOM UTF-8 e finais de linha CRLF ou apenas CR
- `ROTULO + N` além do fim de um `SPACE` e todas as instruções da tabela

`make stress` acrescenta duas ferramentas:

- `scripts/differential.py` gera 3.000 programas válidos aleatórios, calcula seus objetos de forma independente e exige igualdade byte a byte
- `scripts/fuzz.py` submete 6.000 programas aleatórios ou mutados e 600 ligações aleatórias às ferramentas e falha em qualquer travamento

Ambas também foram executadas com AddressSanitizer e UBSan.

## Estrutura do repositório

- `src/` — montador, ligador e código compartilhado
- `tests/` — casos `valid/`, `invalid/`, `link/` e `sim/` com seus resultados esperados
- `examples/` — exemplos da disciplina (`bin.asm` e o exemplo de dois módulos)
- `docs/` — [arquitetura](docs/pt-BR/ARQUITETURA.md), [formatos de objeto](docs/pt-BR/FORMATO_OBJETO.md) (inglês em `docs/`) e o PDF original do enunciado
- `report/` — relatório IEEE em LaTeX, em português (`main_pt_br.tex`) e inglês (`main.tex`)
- `scripts/` — executor dos testes, fuzzer e teste diferencial
- `tools/simulador/` — simulador da disciplina (Linux e Windows)
- `Projeto_Instrucoes.md` / `Project-Instructions.md` — o enunciado, organizado em português e inglês

## Relatório

O relatório segue o formato IEEE de conferência, com o modelo da UnB. A versão em português, entregue na disciplina, é `report/main_pt_br.tex`, e a versão em inglês é `report/main.tex`. Os PDFs compilados são artefatos de build e não são versionados; use `make report` para gerá-los na mesma pasta.

## Licença

Este repositório é destinado ao uso acadêmico no trabalho de Software Básico.

---
Os arquivos deste projeto são distribuídos nos termos do arquivo `LICENSE` incluído (MIT por padrão). Consulte o `LICENSE` para mais detalhes.
