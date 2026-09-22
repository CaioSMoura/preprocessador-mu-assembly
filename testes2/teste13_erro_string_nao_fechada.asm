# Teste 13 - erro isolado: cadeia de caracteres nao fechada

.data
msg1: .asciiz "Mensagem que nunca fecha
msg2: .asciiz "Outra mensagem sem fechamento

.text
main:
    syscall
