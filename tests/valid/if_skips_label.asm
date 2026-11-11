; the label on the line dropped by a false IF does not exist, and a label alone
; before it is dropped together with the statement
F: EQU 0
SECTION TEXT
IF F
L1:
STOP
L1: STOP
JMP L1
