MAIN="${1:-./main.exe}"
DIR="testes_sintatico"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

# arquivo:codigo_de_saida_esperado:erros_sintaticos_esperados
CASOS="
p2_valido_direto_text:0:0
p2_valido_data_text:0:0
p2_valido_alternancia_secao_vazia:0:0
p2_valido_todas_diretivas:0:0
p2_valido_rotulos:0:0
p2_erro_word_virgula_final:1:1
p2_erro_instrucao_em_data:1:1
p2_erro_diretiva_em_text:1:1
p2_erro_token_antes_secao:1:1
p2_erro_rotulo_sem_dois_pontos:1:2
p2_erro_token_sobrando:1:3
p2_erro_operandos_diretivas:1:4
"

falhas=0
for caso in $CASOS; do
    nome="${caso%%:*}"; resto="${caso#*:}"
    rc_esp="${resto%%:*}"; err_esp="${resto#*:}"
    "$MAIN" "$DIR/$nome.asm" "$TMP/$nome" > /dev/null 2>&1
    rc=$?
    erros=$(grep -c '^Linha' "$TMP/$nome.sin.err" 2>/dev/null)
    if [ "$rc" = "$rc_esp" ] && [ "$erros" = "$err_esp" ]; then
        echo "PASSOU  $nome"
    else
        echo "FALHOU  $nome (saida $rc, esperado $rc_esp; erros $erros, esperado $err_esp)"
        falhas=$((falhas + 1))
    fi
done
