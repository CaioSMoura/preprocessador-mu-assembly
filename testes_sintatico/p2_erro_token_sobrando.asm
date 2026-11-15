# Token sobrando depois de construcao completa, em 3 linhas diferentes (2, 3 e 5)
.data extra
valor: .word 1 2
.text
.globl main extra
main:   nop
