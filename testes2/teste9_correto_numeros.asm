.data
zero:      .word 0
pequeno:   .word 15
grande:    .word 2025
negativo:  .word -32768
hexa1:     .word 0x10
hexa2:     .word 0xFF
hexa3:     .word 0x10010000
hexaneg:   .word -0x10

.text
main:
    li $t0, 25
    li $t1, -4
    li $t2, 0xFF
    li $t3, -0x20
    li $v0, 10
    syscall
