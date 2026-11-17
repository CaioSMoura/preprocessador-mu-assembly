# Todas as operacoes de todos os grupos de instrucoes da gramatica
.data
numero: .word 10
msg:    .asciiz "ok"

.text
main:
    # tres registradores
    add  $t0, $t1, $t2
    addu $t0, $t1, $t2
    sub  $t0, $t1, $t2
    subu $t0, $t1, $t2
    mul  $t0, $t1, $t2
    and  $t0, $t1, $t2
    or   $t0, $t1, $t2
    xor  $t0, $t1, $t2
    nor  $t0, $t1, $t2
    slt  $t0, $t1, $t2
    # dois registradores
    move $t0, $t1
    div  $t0, $t1
    # um registrador
    mflo $t0
    mfhi $t1
    jr   $ra
    # imediato
    addi $t0, $t1, 5
    # deslocamento de bits
    sll  $t0, $t1, 2
    srl  $t0, $t1, 2
    # carregamento de valor e endereco
    li   $v0, 4
    la   $a0, msg
    # acesso a memoria
    lw   $t0, numero
    sw   $t0, 0($sp)
    lb   $t0, 1($sp)
    sb   $t0, 2($sp)
    lh   $t0, 4($sp)
    sh   $t0, 6($sp)
    # desvios condicionais
    beq  $t0, $t1, fim
    bne  $t0, $t1, fim
    blt  $t0, $t1, fim
    ble  $t0, $t1, fim
    bgt  $t0, $t1, fim
    bge  $t0, $t1, fim
    # saltos
    j    fim
    jal  fim
    # sem operandos
    nop
fim:
    syscall
