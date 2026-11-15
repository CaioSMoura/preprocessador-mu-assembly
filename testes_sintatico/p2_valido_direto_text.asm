# Programa que comeca direto em .text (sem secao .data)
.text
.globl main
main:
        li   $v0, 10
        syscall
