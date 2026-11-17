# Enderecos mal formados: 4 erros, um por linha
.text
    lw $t0, 4 $sp
    sw $t0, 4($sp
    lb $t0, ($sp)
    sh $t0, 4(10)
    lw $t0, 4($sp)
