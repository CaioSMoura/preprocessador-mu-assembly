#include <stdio.h>
#include <string.h>
#include "preprocessador.h"
#include "lexico.h"

#define SUCESSO 0
#define ERRO_ARGUMENTOS 1
#define ERRO_ENTRADA 2
#define ERRO_SAIDA 3

#define MAX_CAMINHO 512

static void trocar_extensao(const char *origem, const char *ext, char *destino)
{
    const char *ponto = strrchr(origem, '.');
    size_t n = (ponto != NULL) ? (size_t)(ponto - origem) : strlen(origem);
 
    memcpy(destino, origem, n);
    destino[n] = '\0';
    strcat(destino, ext);
}

static void exibir_uso(const char *programa){

    fprintf(stderr, "Uso: %s <arquivo_entrada.asm> <arquivo_saida.lex> \n", programa);
    fprintf(stderr, "Exemplo: %s teste.asm teste.pre\n", programa); // Forma de 3 args
    fprintf(stderr, "Exemplo: %s teste.asm teste.lex\n", programa);
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
        
        strcpy(nome_entrada, argv[1]);
        strcpy(nome_lex, argv[2]);
        trocar_extensao(nome_entrada, ".pre", nome_pre);

    } else if (argc == 4){

        strcpy(nome_entrada, argv[1]);
        strcpy(nome_pre, argv[2]);
        strcpy(nome_lex, argv[3]);

    } else {

        fprintf(stderr, "ERRO: Quantidade de argumentos invalida ");
        fprintf(stderr, "(esperado 2 ou 3, recebido %d).\n", argc - 1);
        
        exibir_uso(argv[0]);
 
        return ERRO_ARGUMENTOS;

    }

    trocar_extensao(nome_lex, ".ts",  nome_ts);
    trocar_extensao(nome_lex, ".err", nome_err);

    entrada = fopen(nome_entrada, "r");

    if (entrada == NULL){

        fprintf(stderr, "ERRO: Nao foi possivel abrir o arquivo de entrada.");
        fprintf(stderr, "\"%s\".\n", nome_entrada);

        return ERRO_ENTRADA;
    }

    saida = fopen(nome_pre, "w");

    if (saida == NULL){

        fprintf(stderr, "ERRO: Nao foi possivel abrir o arquivo de saida");
        fprintf(stderr, "\"%s\".\n", nome_pre);
        fclose(entrada);

        return ERRO_SAIDA;
    }


    PreProcessamento(entrada, saida);

    fclose(entrada);
    fclose(saida);

    entrada = fopen(nome_pre, "r");

    if(entrada == NULL){

        fprintf(stderr, "ERRO: Nao foi possivel reabrir o arquivo ");
        fprintf(stderr, "\"%s\".\n", nome_pre);
 
        return ERRO_ENTRADA;

    }

    saida = fopen(nome_lex, "w");

    if(saida == NULL){

        fprintf(stderr, "ERRO: Nao foi possivel criar o arquivo ");
        fprintf(stderr, "\"%s\".\n", nome_lex);
        fclose(entrada);
 
        return ERRO_SAIDA;

    }

    AnaliseLexica(entrada, saida);

    fclose(entrada);
    fclose(saida);

    saida = fopen(nome_ts, "w");

    if(saida == NULL){

        fprintf(stderr, "ERRO: Nao foi possivel criar o arquivo ");
        fprintf(stderr, "\"%s\".\n", nome_ts);
 
        return ERRO_SAIDA;

    }

    tsGravar(saida);
    fclose(saida);

    saida = fopen(nome_err, "w");

    if(saida == NULL){

        fprintf(stderr, "ERRO: Nao foi possivel criar o arquivo ");
        fprintf(stderr, "\"%s\".\n", nome_err);
 
        return ERRO_SAIDA;

    }

    errosGravar(saida);
    fclose(saida);

    printf("Processamento concluido.\n");
    printf("Entrada: %s\n", nome_entrada);
    printf("Pre-processado:    %s\n", nome_pre);
    printf("Tokens:            %s\n", nome_lex);
    printf("Tabela de simbolos:%s\n", nome_ts);
    printf("Erros lexicos:     %s\n", nome_err);

    return SUCESSO;

}