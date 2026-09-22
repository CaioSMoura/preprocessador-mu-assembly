# Teste de comentarios: comentario de linha inteira antes do codigo

.data
# comentario de linha inteira antes de uma diretiva
msg: .asciiz "Valor: #10"   # comentario apos codigo; o # dentro da string acima deve ser preservado

.text
main:                        # comentario ao final da linha do rotulo
    li $v0, 4                # carrega o codigo de syscall de impressao
    la $a0, msg
    syscall                  # chama o sistema

    # comentario de linha inteira antes do encerramento
    li $v0, 10
    syscall
