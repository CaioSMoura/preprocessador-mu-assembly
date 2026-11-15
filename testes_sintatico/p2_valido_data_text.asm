# Secao .data seguida de .text
.data
msg:    .asciiz "Ola, mundo"
valor:  .word 42

.text
.globl main
main:
        li   $v0, 4
        la   $a0, msg
        syscall
        li   $v0, 10
        syscall
