# Operandos fora de ordem: 3 erros, um por linha
.text
    li 10, $t0
    sll $t0, 2, $t1
    beq fim, $t0, $t1
fim:
    syscall
