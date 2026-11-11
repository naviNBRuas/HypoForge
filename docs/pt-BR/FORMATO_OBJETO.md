# Formatos de objeto

> English: [../OBJECT_FORMAT.md](../OBJECT_FORMAT.md)

## 1. Objeto simples — `montador prog.asm` → `prog.OBJ`

Uma linha de texto. Palavras separadas por espaço, programa primeiro, depois dados `CONST`/`SPACE` (`SPACE` = `0`).
Cada palavra é seguida de um espaço e não há quebra de linha final (layout do `bin.obj` de referência).

```
12 29 10 29 4 28 11 30 3 28 11 31 10 29 2 31 11 31 13 31 9 30 29 10 29 7 4 14 2 0 0 0 
```

## 2. Objeto modular — `montador --modular prog.asm` (usado pelo `ligador`)

O enunciado deixa este formato livre. Layout (segue o exemplo `LIG.OBJ.txt` do curso):

```
D, <nome> <endereço>     uma linha por símbolo PUBLIC (endereço dentro do módulo)
U, <nome> <endereço>     uma linha por uso de símbolo EXTERN (endereço da palavra do operando)
R, <b0> <b1> ... <bn>    um bit de realocação por palavra: 1 = endereço que depende da base de carga
<w0> <w1> ... <wn>       a linha de código (igual ao objeto simples)
```

Exemplo — `examples/modulo1.asm` (11 palavras):

```
D, L2 6
D, OLD_DATA 8
D, NEW_DATA 9
D, TMP_DATA 10
U, MOD2 5
R, 0 1 0 1 0 0 0 0 0 0 0
12 8 10 8 5 0 14 2 0 0 0
```

Convenções:

* Palavra que referencia rótulo **local** guarda o endereço relativo ao módulo e tem `R = 1`.
* Palavra que referencia símbolo **externo** guarda apenas o adendo (`0`, ou `N` em `X + N`), tem `R = 0` e aparece numa linha `U`.
* Opcodes e dados `CONST`/`SPACE` têm `R = 0`.

## 3. Executável ligado — `ligador a.asm b.asm c.asm` → `a.LIG`

Mesmo formato do objeto simples. Para cada módulo `m` com base `B(m)` (soma dos tamanhos dos módulos anteriores):

```
final[w] = code[w] + B(m)                      se R[w] = 1
final[w] = code[w] + endereço(símbolo)         para cada entrada U (símbolo, w)
endereço(símbolo) = B(módulo que define) + endereço D
```

Os módulos são dispostos na ordem da linha de comando; o módulo 1 começa no endereço 0, onde o simulador inicia a execução.
Cada módulo mantém seus dados logo após o seu próprio código.

Exemplo (`examples/modulo1.asm` + `examples/modulo2.asm`, bases 0 e 11):

```
12 8 10 8 5 11 14 2 0 0 0 | 4 36 11 9 3 36 11 10 10 8 2 10 11 10 13 10 9 9 8 10 8 7 11 5 6 2
```
