#include <stdio.h>
#include <string.h>
#include "preprocessador.h"
#include "lexico.h"

#define SUCESSO 0
#define ERRO_ARGUMENTOS 1
#define ERRO_ENTRADA 2
#define ERRO_SAIDA 3

#define MAX_CAMINHO 512

static int copiar_caminho(char *destino, const char *origem)
{
    if (strlen(origem) >= MAX_CAMINHO) {
        fprintf(stderr, "ERRO: Caminho muito longo (maximo %d caracteres): \"%s\".\n",
                MAX_CAMINHO - 1, origem);
        return 0;
    }

    strcpy(destino, origem);
    return 1;
}

static int trocar_extensao(const char *origem, const char *ext, char *destino)
{
    const char *ponto = strrchr(origem, '.');
    const char *barra = strrchr(origem, '/');
    const char *contrabarra = strrchr(origem, '\\');

    if (contrabarra != NULL && (barra == NULL || contrabarra > barra)) {
        barra = contrabarra;
    }

    if (ponto != NULL && barra != NULL && ponto < barra) {
        ponto = NULL;
    }

    size_t n = (ponto != NULL) ? (size_t)(ponto - origem) : strlen(origem);

    if (n + strlen(ext) >= MAX_CAMINHO) {
        fprintf(stderr, "ERRO: Caminho muito longo para gerar \"%s\".\n", ext);
        return 0;
    }

    memcpy(destino, origem, n);
    destino[n] = '\0';
    strcat(destino, ext);

    return 1;
}

static int fechar_arquivo(FILE *arq, const char *nome, int gravacao)
{
    int ok = 1;

    if (ferror(arq)) {
        fprintf(stderr, "ERRO: Falha na %s do arquivo \"%s\".\n",
                gravacao ? "gravacao" : "leitura", nome);
        ok = 0;
    }

    if (fclose(arq) != 0) {
        fprintf(stderr, "ERRO: Falha ao fechar o arquivo \"%s\".\n", nome);
        ok = 0;
    }

    return ok;
}

static void exibir_uso(const char *programa){

    fprintf(stderr, "Uso: %s <arquivo_entrada.asm> <arquivo_saida.lex>\n", programa);
    fprintf(stderr, "  ou %s <arquivo_entrada.asm> <arquivo_saida.pre> <arquivo_saida.lex>\n", programa);
    fprintf(stderr, "Exemplo: %s teste.asm teste.lex\n", programa);
    fprintf(stderr, "Exemplo: %s teste.asm teste.pre teste.lex\n", programa);
}

int main(int argc, char *argv[]){

    char nome_entrada[MAX_CAMINHO];
    char nome_pre[MAX_CAMINHO];
    char nome_lex[MAX_CAMINHO];
    char nome_ts[MAX_CAMINHO];
    char nome_err[MAX_CAMINHO];

    FILE *entrada = NULL;
    FILE *saida = NULL;

    if(argc == 3){

        if (!copiar_caminho(nome_entrada, argv[1]) ||
            !copiar_caminho(nome_lex, argv[2]) ||
            !trocar_extensao(nome_entrada, ".pre", nome_pre)) {
            return ERRO_ARGUMENTOS;
        }

    } else if (argc == 4){

        if (!copiar_caminho(nome_entrada, argv[1]) ||
            !copiar_caminho(nome_pre, argv[2]) ||
            !copiar_caminho(nome_lex, argv[3])) {
            return ERRO_ARGUMENTOS;
        }

    } else {

        fprintf(stderr, "ERRO: Quantidade de argumentos invalida ");
        fprintf(stderr, "(esperado 2 ou 3, recebido %d).\n", argc - 1);

        exibir_uso(argv[0]);

        return ERRO_ARGUMENTOS;

    }

    if (!trocar_extensao(nome_lex, ".ts",  nome_ts) ||
        !trocar_extensao(nome_lex, ".err", nome_err)) {
        return ERRO_ARGUMENTOS;
    }


    entrada = fopen(nome_entrada, "r");

    if (entrada == NULL){

        fprintf(stderr, "ERRO: Nao foi possivel abrir o arquivo de entrada ");
        fprintf(stderr, "\"%s\".\n", nome_entrada);

        return ERRO_ENTRADA;
    }

    saida = fopen(nome_pre, "w");

    if (saida == NULL){

        fprintf(stderr, "ERRO: Nao foi possivel criar o arquivo ");
        fprintf(stderr, "\"%s\".\n", nome_pre);
        fclose(entrada);

        return ERRO_SAIDA;
    }

    PreProcessamento(entrada, saida);

    if (!fechar_arquivo(entrada, nome_entrada, 0)) {
        fclose(saida);
        preLiberarMapa();
        return ERRO_ENTRADA;
    }

    if (!fechar_arquivo(saida, nome_pre, 1)) {
        preLiberarMapa();
        return ERRO_SAIDA;
    }


    entrada = fopen(nome_pre, "r");

    if(entrada == NULL){

        fprintf(stderr, "ERRO: Nao foi possivel reabrir o arquivo ");
        fprintf(stderr, "\"%s\".\n", nome_pre);
        preLiberarMapa();

        return ERRO_ENTRADA;

    }

    saida = fopen(nome_lex, "w");

    if(saida == NULL){

        fprintf(stderr, "ERRO: Nao foi possivel criar o arquivo ");
        fprintf(stderr, "\"%s\".\n", nome_lex);
        fclose(entrada);
        preLiberarMapa();

        return ERRO_SAIDA;

    }

    AnaliseLexica(entrada, saida);

    if (!fechar_arquivo(entrada, nome_pre, 0)) {
        fclose(saida);
        preLiberarMapa();
        return ERRO_ENTRADA;
    }

    if (!fechar_arquivo(saida, nome_lex, 1)) {
        preLiberarMapa();
        return ERRO_SAIDA;
    }


    saida = fopen(nome_ts, "w");

    if(saida == NULL){

        fprintf(stderr, "ERRO: Nao foi possivel criar o arquivo ");
        fprintf(stderr, "\"%s\".\n", nome_ts);
        preLiberarMapa();

        return ERRO_SAIDA;

    }

    tsGravar(saida);

    if (!fechar_arquivo(saida, nome_ts, 1)) {
        preLiberarMapa();
        return ERRO_SAIDA;
    }


    saida = fopen(nome_err, "w");

    if(saida == NULL){

        fprintf(stderr, "ERRO: Nao foi possivel criar o arquivo ");
        fprintf(stderr, "\"%s\".\n", nome_err);
        preLiberarMapa();

        return ERRO_SAIDA;

    }

    errosGravar(saida);

    if (!fechar_arquivo(saida, nome_err, 1)) {
        preLiberarMapa();
        return ERRO_SAIDA;
    }

    preLiberarMapa();

    printf("Processamento concluido.\n");
    printf("Entrada:            %s\n", nome_entrada);
    printf("Pre-processado:     %s\n", nome_pre);
    printf("Tokens:             %s\n", nome_lex);
    printf("Tabela de simbolos: %s\n", nome_ts);
    printf("Erros lexicos:      %s\n", nome_err);

    return SUCESSO;

}