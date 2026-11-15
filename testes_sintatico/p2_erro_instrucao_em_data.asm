# Instrucao dentro de .data (erro na linha 4); a secao .text depois continua valida
.data
msg:    .asciiz "ok"
        add  $t0, $t1, $t2
.text
main:   nop
