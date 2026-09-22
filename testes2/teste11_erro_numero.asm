# Teste 11 - erro isolado: numero malformado

.text
main:
    li $t0, 0x
    li $t1, -0x
    li $t2, 0X
    syscall
