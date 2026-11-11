# HypoForge — Montador e Ligador para uma Máquina Hipotética

[English](README.md) | Português

**HypoForge** é um **montador** (`montador`) de duas passagens e um **ligador** (`ligador`) para a máquina hipotética de 14 instruções da disciplina *Software Básico*
(Universidade de Brasília, UnB), escritos em C++17 sem dependências externas.

- `montador` — traduz `.asm` para um objeto texto de uma linha (`.OBJ`), reportando erros **léxicos, sintáticos e semânticos** com as linhas
- `ligador` — liga até três módulos (`BEGIN`/`END`/`PUBLIC`/`EXTERN`) em um executável `.LIG`
- suporte completo a `SPACE n`, `CONST`, endereçamento `ROTULO + N`, `EQU`/`IF`, comentários, insensibilidade ao caso e rótulos em linha separada
- suíte de testes com resultados esperados calculados à mão, além de execuções ponta a ponta no simulador da disciplina

## Início rápido

```
make                                   # gera ./montador e ./ligador
./montador examples/bin.asm            # -> examples/bin.OBJ
./ligador examples/modulo1.asm examples/modulo2.asm   # -> examples/modulo1.LIG
echo 13 | tools/simulador/simulador examples/modulo1.LIG   # imprime 1 0 1 1 (dígitos binários, LSB primeiro)
```

Requisitos: compilador C++17 (`g++` ≥ 8, MinGW funciona) e `make`. LaTeX (`pdflatex`, `latexmk`, IEEEtran) só é necessário para o relatório.

## Reprodutibilidade (passo a passo)

1. Compilar: `make`
2. Rodar toda a suíte: `make test` (programas válidos/inválidos do montador, casos do ligador, execuções no simulador)
3. Montar o exemplo fornecido e comparar com o objeto de referência:
   `./montador examples/bin.asm && cmp examples/bin.OBJ examples/bin.expected.OBJ`
4. Provocar erros: `./montador tests/invalid/semantic.asm`
5. Gerar os relatórios (PDF): `make report` → `report/main_pt_br.pdf` (português) e `report/main.pdf` (inglês)

O simulador em `tools/simulador/` é o fornecido pela disciplina (`simulador` para Linux, `simulador.exe` para Windows);
se não executar na sua máquina, `make test` marca esses casos como *skipped*.

## Resumo do comportamento

| Ferramenta | Entrada | Saída |
|------------|---------|-------|
| `./montador prog.asm` | arquivo assembly | `prog.OBJ` — uma linha, palavras separadas por espaço, programa primeiro e depois dados `CONST`/`SPACE` |
| `./ligador a.asm b.asm c.asm` | 1–3 módulos | `a.OBJ`, `b.OBJ`, `c.OBJ` (objetos modulares) e `a.LIG` (executável) |

Havendo erros, o montador analisa o arquivo **inteiro** e imprime, para cada problema,
`[ERRO <LEXICO|SINTATICO|SEMANTICO>] linha N: mensagem` seguido da linha do fonte; nenhum `.OBJ` é gerado.

```
$ ./montador tests/invalid/spec_example.asm
[ERRO SEMANTICO] linha 11: rotulo 'N3' nao declarado
    11 | COPY N2 N3
...
```

## Comandos

- `make` — compila
- `make test` — executa `scripts/run_tests.sh`
- `make report` — gera `report/main_pt_br.pdf` e `report/main.pdf`
- `make clean` — remove binários e objetos

## Estrutura do repositório

- `src/` — montador, ligador e código compartilhado
- `tests/` — casos `valid/`, `invalid/`, `link/`, `sim/` com seus resultados esperados
- `examples/` — exemplos da disciplina (`bin.asm`, exemplo de dois módulos)
- `docs/` — [arquitetura](docs/pt-BR/ARQUITETURA.md), [formatos de objeto](docs/pt-BR/FORMATO_OBJETO.md) (inglês em `docs/`) e o PDF original do enunciado
- `report/` — relatório IEEE em LaTeX (modelo UnB): `main_pt_br.tex` em português, `main.tex` em inglês
- `scripts/` — executor dos testes
- `tools/simulador/` — simulador da disciplina (Linux e Windows)
- `Projeto_Instrucoes.md` / `Project-Instructions.md` — o enunciado, organizado em português e inglês

## Relatório

O relatório segue o formato IEEE de conferência (modelo UnB). A versão em português, entregue na disciplina,
é `report/main_pt_br.tex`, e a versão em inglês é `report/main.tex`.
Os PDFs compilados são artefatos de build e não são versionados; use `make report` para gerá-los.

## Licença

MIT — veja [LICENSE](LICENSE). Destinado a uso acadêmico.
