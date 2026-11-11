; modulo2.asm - second module of the linking example.
; Together with modulo1.asm it behaves like bin.asm: prints the binary digits
; (least significant first) of the number read from the input.
;   ./ligador examples/modulo1.asm examples/modulo2.asm
MODULO2: BEGIN
PUBLIC MOD2
L2: EXTERN
OLD_DATA: EXTERN
NEW_DATA: EXTERN
TMP_DATA: EXTERN

SECTION TEXT
MOD2: DIV DOIS          ; the accumulator already holds OLD_DATA (loaded by module 1)
STORE NEW_DATA
MULT DOIS
STORE TMP_DATA
LOAD OLD_DATA
SUB TMP_DATA
STORE TMP_DATA
OUTPUT TMP_DATA
COPY NEW_DATA OLD_DATA
LOAD OLD_DATA
JMPP MOD2
JMP L2                  ; back to the STOP in module 1
END

SECTION DATA
DOIS: CONST 2
