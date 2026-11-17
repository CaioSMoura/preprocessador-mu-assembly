# Rotulo seguido de instrucao na mesma linha, rotulo isolado,
# referencia antecipada, rotulo inexistente e rotulo duplicado
# (os dois ultimos sao questoes semanticas e nao devem rejeitar o programa)
.text
inicio: li $t0, 10
loop:   addi $t0, $t0, -1
        bne $t0, $zero, loop
        beq $t0, $zero, depois
        j rotulo_que_nao_existe
depois:
        jal funcao
funcao: jr $ra
loop:   nop
fim:    syscall
