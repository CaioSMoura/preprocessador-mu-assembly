# Operandos de categoria inadequada: 5 erros, um por linha
.data
mensagem: .asciiz "texto"

.text
    li $t0, mensagem
    la $t0, 10
    j 100
    beq $t0, $t1, 5
    addi $t0, $t1, $t2
    nop
