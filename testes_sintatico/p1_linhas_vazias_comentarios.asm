# Linhas vazias e comentarios nao geram EOL extra

.data

# comentario isolado
msg: .asciiz "a, b> 1 2" # string com virgula e >


.text
main:   li $v0, 10   # comentario no fim

        syscall
