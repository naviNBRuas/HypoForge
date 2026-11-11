section data
n1:	space
dois: const  2
;a comment line

SECAO TEXT
Rot:
   iNput   N1       ; label alone on the previous line
	copy n1 N1
	Load N1
	div Dois
	OUTPUT n1
	Jmpz rot
	stop
