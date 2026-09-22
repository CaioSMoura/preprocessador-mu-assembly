# Teste 12 - erro isolado: caractere invalido

.text
main:
    add $t1, $t0, @
    add $t1, $t0, %
    add $t1, $t0, !
    syscall
