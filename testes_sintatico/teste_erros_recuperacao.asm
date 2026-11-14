.data
    valido1: .word 10, 20
    erro1: .word 10,
    valido2: .asciiz "Linha recuperada com sucesso"

.text
main:
    add $t0, $t1, $t2
    add $t0 $t1, $t2
    sub $t3, $t4, $t5
    lw $t0, 4 $sp
    li $t1, 100
    syscall