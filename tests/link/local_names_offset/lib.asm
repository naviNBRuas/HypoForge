lib: begin            ; lower case directives
public vet
public soma
public res
volta: extern
section text
soma: load vet
add vet + 1
store t
copy t res
jmp volta
end
section data
vet: space 2
t: space
res: space
