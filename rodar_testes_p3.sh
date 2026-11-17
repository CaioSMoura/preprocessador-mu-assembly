MAIN="${1:-./main.exe}"
DIR="testes_sintatico"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

# arquivo:codigo_de_saida_esperado:erros_sintaticos_esperados
CASOS="
p3_valido_todas_instrucoes:0:0
p3_valido_numeros_e_enderecos:0:0
p3_valido_rotulo_e_instrucao:0:0
p3_erro_faltam_operandos:1:3
p3_erro_operando_excedente:1:3
p3_erro_categoria_errada:1:5
p3_erro_virgula_ausente:1:3
p3_erro_endereco:1:4
p3_erro_fora_de_ordem:1:3
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
