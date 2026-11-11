; a label alone, followed by comment-only and blank lines, still labels the next statement
SECTION TEXT
ROT: ; only a comment

   ; another comment
	INPUT X
JMP ROT
SECTION DATA
X: SPACE
