.data
msg1: .asciiz "Ola mundo"
msg2: .asciiz "Linha 1\nLinha 2"
msg3: .asciiz "Coluna A\tColuna B"
msg4: .asciiz "Ela disse: \"ola\""
msg5: .asciiz "Caminho: C:\\dados"
msg6: .asciiz "Fim\0"

.text
main:
    la $a0, msg1
    li $v0, 4
    syscall
