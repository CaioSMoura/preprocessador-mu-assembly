# As 10 diretivas: .data .text .globl .word .half .byte .ascii .asciiz .space .align
.data
.globl numeros
numeros: .word 1, 0x2A, -3
metade:  .half 4, 5
bytes:   .byte 7, 0x0F
texto:   .ascii "sem terminador"
frase:   .asciiz "aspas \" e barra \\ ok"
         .space 16
         .align 2
.text
.globl main
main:
