.data
    msg_aspas: .asciiz "Texto com \"aspas\" internas"
    msg_barra: .asciiz "C:\\pasta\\arquivo.asm"

.text
inicio:
    la $a0, msg_aspas
    li $v0, 4
    syscall
inicio:
    j rotulo_inexistente