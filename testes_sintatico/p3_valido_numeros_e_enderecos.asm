# Inteiros decimais, negativos e hexadecimais; enderecos por identificador
# e por deslocamento(registrador); registradores na forma numerica
.data
vetor: .word 10, -20, 0xFF, -0x10

.text
    li   $t0, 0
    li   $t1, 2025
    li   $t2, -32768
    li   $t3, 0x10010000
    li   $t4, -0x10
    addi $t5, $t5, -1
    addi $t6, $t6, 0xFF
    sll  $t7, $t7, 0
    lw   $t0, vetor
    sw   $t0, vetor
    lw   $t1, 0($sp)
    lw   $t1, -4($fp)
    sw   $t2, 0x10($gp)
    lb   $8, 3($29)
    la   $a0, vetor
    add  $0, $31, $16
