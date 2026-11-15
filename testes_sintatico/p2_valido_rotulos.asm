# Rotulo isolado, rotulo + diretiva, rotulo + instrucao, .globl nas duas secoes
.data
inicio:
dado:   .word 10
.globl dado
.text
.globl principal
principal:
laco:   addi $t0, $t0, 1
        j    laco
fim:
        syscall
