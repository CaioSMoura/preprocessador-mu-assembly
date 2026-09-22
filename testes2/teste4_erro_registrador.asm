# Teste 4 - erro isolado: registrador invalido

.text
main:
    li $t0, 1
    li $t10, 2
    li $32, 3
    li $s9, 4
    move $x1, $t0
    syscall