# Teste 14 - erro isolado: sequencia de escape invalida

.data
msg1: .asciiz "Escape invalido \q aqui"
msg2: .asciiz "Outro invalido \z exemplo"
msg3: .asciiz "Numero errado \9 no texto"

.text
main:
    syscall
