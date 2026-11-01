# Rotulos usando palavras reservadas devem gerar ERRO_ROTULO_RESERVADO
.text
add: li $t0, 1
ADD : nop
.data: nop
$t0: nop
loop: j loop
