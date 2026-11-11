# Arquitetura

> English: [../ARCHITECTURE.md](../ARCHITECTURE.md)

Dois executáveis são gerados a partir dos fontes em `src/`:

| Binário | Ponto de entrada | Função |
|---------|------------------|--------|
| `montador` | `montador_main.cpp` | Monta um `.asm` em `.OBJ` |
| `ligador` | `ligador_main.cpp` | Chama `montador --modular` para cada módulo e liga tudo em `.LIG` |

```
src/
├── common.{hpp,cpp}     tabela de instruções (14 opcodes), lista de diretivas, tipos de erro
├── lexer.{hpp,cpp}      linha -> tokens, erros léxicos
├── assembler.{hpp,cpp}  análise sintática, tabela de símbolos, checagens semânticas, geração de código
├── objfile.{hpp,cpp}    formatos de objeto (simples / modular), utilitários de arquivo
├── linker.{hpp,cpp}     realocação + resolução de símbolos externos
├── montador_main.cpp    CLI do montador
└── ligador_main.cpp     CLI do ligador
```

## Pipeline de montagem (duas passagens)

1. **Análise léxica** (`lexer.cpp`) — cada linha é cortada em `;` (comentário) e dividida em tokens
   (`Ident`, `Number`, `:`, `+`, `-`, `,`). Identificadores viram maiúsculos, tornando a linguagem
   insensível ao caso. Tokens ruins (caractere inválido, identificador com mais de 30 caracteres,
   token começando por dígito como `1ABC` ou `0x10`) geram erro **LEXICO**.
2. **Passagem 1 — sintaxe + símbolos** (`Assembler::parseLine`) — reconhece `rotulo:`, instruções e
   diretivas, confere a quantidade de operandos, acompanha a seção atual e registra cada rótulo na
   tabela de símbolos com endereço *relativo à seção*. Rótulo sozinho na linha fica *pendente* e é
   associado ao próximo comando. `EQU`/`IF` são resolvidos aqui: `IF` falso descarta a linha seguinte.
3. **Passagem 2 — semântica + código** (`Assembler::generate`) — com o tamanho do texto conhecido, rótulos
   de dados recebem `endereço + tamanhoTexto`. Os operandos são resolvidos (`ROTULO + N`), as checagens
   semânticas rodam e as palavras são emitidas: **programa primeiro, depois dados CONST/SPACE**,
   independentemente da ordem das seções no arquivo.

O arquivo inteiro é sempre analisado; os erros são coletados, ordenados por linha e exibidos. **Nenhum `.OBJ` é gerado se houver erro.**

## Diagnósticos

| Classe | Exemplos reportados |
|--------|---------------------|
| **LEXICO** | caractere inválido; identificador > 30 caracteres; token começando por dígito (`1ABC`, hexadecimal `0x10`); número fora do intervalo |
| **SINTATICO** | quantidade de operandos errada (instruções, `SPACE`, `CONST`, `PUBLIC`…); instrução/diretiva desconhecida; vírgula entre operandos; dois rótulos no mesmo comando; argumento inválido de `SECTION`; rótulo sem comando |
| **SEMANTICO** | rótulo não declarado; rótulo/seção duplicado; seção `TEXT` ausente; instrução em `DATA` / `CONST`,`SPACE` em `TEXT`; `EQU` após seção; `IF` fora de `TEXT`; salto para rótulo de dados; rótulo de código usado como dado; escrita em `CONST`; divisão por constante `0`; `BEGIN` sem `END` (e vice-versa); símbolo `PUBLIC` não definido |

Linha que já tem erro léxico não recebe análise sintática adicional (evita cascata) e mensagens idênticas na mesma linha são reportadas uma vez.

## Decisões de interpretação

O enunciado deixa alguns pontos em aberto. Estas são as escolhas feitas (todas cobertas por `tests/`):

| Tema | Decisão |
|------|---------|
| Palavra da seção | `SECAO` (enunciado) e `SECTION` (exemplos do Moodle) são aceitas |
| `COPY A, B` | Erro sintático (SINTATICO): vírgula não faz parte da linguagem; os testes nunca são "corrigidos" |
| `CONST` hexadecimal | Rejeitado como LEXICO — o enunciado diz *somente decimal* |
| Bytes da saída | Cada palavra seguida de um espaço, sem quebra de linha final — idêntico ao `bin.obj` de referência |
| `END` | Não interrompe a leitura (o exemplo do curso coloca `SECTION DATA` depois do `END`); `BEGIN`/`END` devem formar par |
| `NOME: BEGIN` / `BEGIN: NOME` | Ambas aceitas (o exemplo do curso usa a segunda) |
| `NOME: EXTERN` / `EXTERN NOME` | Ambas aceitas |
| Dois rótulos antes de um comando | Erro, inclusive quando separados por quebra de linha |
| `IF` falso | Descarta a próxima linha não vazia |
| Checagens semânticas extras | escrita em const, divisão por constante zero, tipo de rótulo incompatível — casos clássicos além dos 6 listados |
| Nome do `.LIG` | Nome do primeiro módulo (`prog1.asm` → `prog1.LIG`) |

## Ligador

`ligador` executa `montador --modular` em cada arquivo (procurado ao lado do binário `ligador`, depois em `./`, depois no `PATH`),
lê os objetos modulares ([FORMATO_OBJETO.md](FORMATO_OBJETO.md)) e:

1. calcula a **base** de cada módulo (soma dos tamanhos dos anteriores);
2. monta a **tabela global de símbolos** a partir das definições `PUBLIC` (duplicatas são erro);
3. soma a base a cada palavra marcada na linha de **realocação**;
4. para cada **uso** de símbolo `EXTERN`, soma o endereço global do símbolo à palavra;
5. grava o `.LIG` em uma única linha.

Externos não resolvidos e públicos duplicados abortam a ligação com mensagens `[ERRO LIGACAO]`.
