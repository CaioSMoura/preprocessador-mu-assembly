#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "preprocessador.h"
#include "lexico.h"
#include "sintatico.h"

#define SUCESSO          0
#define ERRO_REJEITADO   1   /* erros lexicos ou sintaticos */
#define ERRO_ARGUMENTOS  2
#define ERRO_ENTRADA     2
#define ERRO_SAIDA       2

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

/* Gera o nome de um arquivo de saida: prefixo (origem) + extensao */
static int trocar_extensao(const char *origem, const char *ext, char *destino)
{
    if (strlen(origem) + strlen(ext) >= MAX_CAMINHO) {
        fprintf(stderr, "ERRO: Caminho muito longo para gerar \"%s\".\n", ext);
        return 0;
    }

    strcpy(destino, origem);
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

    fprintf(stderr, "Uso: %s <arquivo_entrada.asm> <prefixo_saida>\n", programa);
    fprintf(stderr, "Exemplo: %s teste.asm resultado\n", programa);
}

int main(int argc, char *argv[]){

    char nome_entrada[MAX_CAMINHO];
    char nome_pre[MAX_CAMINHO];
    char nome_lex[MAX_CAMINHO];
    char nome_ts[MAX_CAMINHO];
    char nome_err[MAX_CAMINHO];
    char nome_sin_err[MAX_CAMINHO];
    char nome_dot[MAX_CAMINHO];

    FILE *entrada = NULL;
    FILE *saida = NULL;
    FILE *arvore = NULL;

    int resultado;

    atexit(lexicoLiberar);

    if(argc != 3){

        fprintf(stderr, "ERRO: Quantidade de argumentos invalida ");
        fprintf(stderr, "(esperado 2, recebido %d).\n", argc - 1);

        exibir_uso(argv[0]);

        return ERRO_ARGUMENTOS;

    }

    if (!copiar_caminho(nome_entrada, argv[1]) ||
        !trocar_extensao(argv[2], ".pre",     nome_pre) ||
        !trocar_extensao(argv[2], ".lex",     nome_lex) ||
        !trocar_extensao(argv[2], ".ts",      nome_ts) ||
        !trocar_extensao(argv[2], ".lex.err", nome_err) ||
        !trocar_extensao(argv[2], ".sin.err", nome_sin_err) ||
        !trocar_extensao(argv[2], ".dot",     nome_dot)) {
        return ERRO_ARGUMENTOS;
    }


    entrada = fopen(nome_entrada, "rb");

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

    saida = fopen(nome_sin_err, "w");

    if(saida == NULL){

        fprintf(stderr, "ERRO: Nao foi possivel criar o arquivo ");
        fprintf(stderr, "\"%s\".\n", nome_sin_err);

        return ERRO_SAIDA;

    }

    arvore = fopen(nome_dot, "w");

    if(arvore == NULL){

        fprintf(stderr, "ERRO: Nao foi possivel criar o arquivo ");
        fprintf(stderr, "\"%s\".\n", nome_dot);
        fclose(saida);

        return ERRO_SAIDA;

    }

    if (lexicoQtdErros() > 0) {

        printf("ANÁLISE SINTÁTICA NÃO EXECUTADA: EXISTEM ERROS LÉXICOS.\n");
        fprintf(saida, "ANÁLISE SINTÁTICA NÃO EXECUTADA: EXISTEM ERROS LÉXICOS.\n");

        resultado = ERRO_REJEITADO;

    } else {

        entrada = fopen(nome_lex, "r");

        if(entrada == NULL){

            fprintf(stderr, "ERRO: Nao foi possivel reabrir o arquivo ");
            fprintf(stderr, "\"%s\".\n", nome_lex);
            fclose(saida);
            fclose(arvore);

            return ERRO_ENTRADA;

        }

        resultado = AnaliseSintatica(entrada, arvore, saida);

        if (!fechar_arquivo(entrada, nome_lex, 0)) {
            fclose(saida);
            fclose(arvore);
            return ERRO_ENTRADA;
        }

        if (resultado == SIN_FALHA) {

            fprintf(stderr, "ERRO: A analise sintatica nao pode ser concluida.\n");
            fclose(saida);
            fclose(arvore);

            return ERRO_ENTRADA;

        }

        if (resultado == SIN_VALIDO) {
            printf("PROGRAMA SINTATICAMENTE VÁLIDO\n");
            resultado = SUCESSO;
        } else {
            printf("PROGRAMA SINTATICAMENTE INVÁLIDO\n");
            resultado = ERRO_REJEITADO;
        }

    }

    if (!fechar_arquivo(arvore, nome_dot, 1)) {
        fclose(saida);
        return ERRO_SAIDA;
    }

    if (!fechar_arquivo(saida, nome_sin_err, 1)) {
        return ERRO_SAIDA;
    }

    return resultado;

}
