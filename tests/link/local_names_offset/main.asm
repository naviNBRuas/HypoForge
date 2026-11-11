; DATA before TEXT, EXTERN used with an offset (VET + 1) and a local label T
; whose name also exists, unexported, in lib.asm: locals never clash.
MAIN: BEGIN
VET: EXTERN
SOMA: EXTERN
RES: EXTERN
PUBLIC VOLTA
SECTION DATA
CEM: CONST 100
T: SPACE
SECTION TEXT
INPUT VET
INPUT VET + 1
JMP SOMA
VOLTA: COPY RES T
LOAD T
ADD CEM
STORE T
OUTPUT T
STOP
END
