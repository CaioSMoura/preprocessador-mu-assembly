#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "preprocessador.h"

void remover_quebra_linha(char *linha){
    if (linha == NULL) {
        return;
    }

    size_t tamanho = strlen(linha);

    while (tamanho > 0 &&
           (linha[tamanho - 1] == '\n' ||
            linha[tamanho - 1] == '\r')) {

        linha[tamanho - 1] = '\0';
        tamanho--;
    }
}

void remover_comentario(char *linha){
    int dentro_string = 0;
    
    for (int i = 0; linha[i] != '\0'; i++) {
        if (dentro_string && linha[i] == '\\' && linha[i + 1] != '\0') {
            i++;
            continue;
        }
        if (linha[i] == '"') {
            dentro_string = !dentro_string;
        } else if (linha[i] == '#' && !dentro_string) {
            linha[i] = '\0';
            return;
        }
    } 
}

void normalizar_espacos_mapa(char *linha, int *cols){
    if (linha == NULL) return;

    int i = 0;
    int j = 0;
    int dentro_string = 0;
    int espaco_pendente = 0;

    // 1. Ignora espacos e tabulacoes no inicio da linha (ltrim)
    while (linha[i] == ' ' || linha[i] == '\t') {
        i++;
    }

    // 2. Percorre a linha caractere por caractere
    while (linha[i] != '\0' && linha[i] != '\n' && linha[i] != '\r') {
        if (dentro_string && linha[i] == '\\' && linha[i + 1] != '\0') {
            if (cols) cols[j] = i + 1;
            linha[j++] = linha[i++];
            if (cols) cols[j] = i + 1;
            linha[j++] = linha[i++];
            continue;
        }
        if (linha[i] == '"') {
            if (espaco_pendente && j > 0) {
                if (cols) cols[j] = i;
                linha[j++] = ' ';
                espaco_pendente = 0;
            }
            dentro_string = !dentro_string;
            if (cols) cols[j] = i + 1;
            linha[j++] = linha[i];
        }
        else if (dentro_string) {
            if (cols) cols[j] = i + 1;
            linha[j++] = linha[i];
        }
        else {
            if (linha[i] == ' ' || linha[i] == '\t') {
                if (j > 0) {
                    espaco_pendente = 1;
                }
            } else {
                if (espaco_pendente) {
                    if (cols) cols[j] = i;
                    linha[j++] = ' ';
                    espaco_pendente = 0;
                }
                if (cols) cols[j] = i + 1;
                linha[j++] = linha[i];
            }
        }
        i++;
    }

    // 3. Finaliza a string sem espacos no final (rtrim)
    linha[j] = '\0';
}

void normalizar_espacos(char *linha){
    normalizar_espacos_mapa(linha, NULL);
}


int linha_vazia(const char *linha){
    if (linha == NULL) {
        return 1;
    }

    while (*linha != '\0') {

        if (*linha != ' ' &&
            *linha != '\t' &&
            *linha != '\r' &&
            *linha != '\n') {

            return 0;
        }

        linha++;
    }

    return 1;
}

typedef struct {
    int linhaOrig;   /* linha do .asm que originou esta linha do .pre */
    int tam;         /* quantidade de caracteres da linha no .pre     */
    int *colOrig;    /* colOrig[j] = coluna original do caractere j   */
} MapaLinha;

static MapaLinha *mapa = NULL;
static int mapaQtd = 0;
static int mapaCap = 0;
static int totalLinhasOrig = 0;
static int mapaAtivo = 0;

void preLiberarMapa(void){
    for (int k = 0; k < mapaQtd; k++) {
        free(mapa[k].colOrig);
    }
    free(mapa);
    mapa = NULL;
    mapaQtd = mapaCap = 0;
    totalLinhasOrig = 0;
    mapaAtivo = 0;
}

static void mapaAdicionar(int linhaOrig, const char *linha, const int *cols){
    if (mapaQtd == mapaCap) {
        int novaCap = (mapaCap == 0) ? 64 : mapaCap * 2;
        MapaLinha *novo = realloc(mapa, (size_t)novaCap * sizeof(MapaLinha));
        if (novo == NULL) {
            fprintf(stderr, "AVISO: memoria insuficiente para o mapa de posicoes.\n");
            return;
        }
        mapa = novo;
        mapaCap = novaCap;
    }

    int tam = (int)strlen(linha);
    MapaLinha *m = &mapa[mapaQtd];

    m->linhaOrig = linhaOrig;
    m->tam = tam;
    m->colOrig = malloc((size_t)(tam > 0 ? tam : 1) * sizeof(int));

    if (m->colOrig == NULL) {
        m->tam = 0;
    } else {
        memcpy(m->colOrig, cols, (size_t)tam * sizeof(int));
    }

    mapaQtd++;
}

void preMapear(int linhaPre, int colunaPre, int *linhaOrig, int *colunaOrig){
    *linhaOrig = linhaPre;
    *colunaOrig = colunaPre;

    if (!USAR_POSICAO_ORIGINAL || !mapaAtivo) return;
    if (linhaPre < 1 || linhaPre > mapaQtd) return;

    MapaLinha *m = &mapa[linhaPre - 1];
    *linhaOrig = m->linhaOrig;

    if (colunaPre >= 1 && colunaPre <= m->tam) {
        *colunaOrig = m->colOrig[colunaPre - 1];
    }
}

void preMapearEOF(int *linhaOrig, int *colunaOrig){
    if (!USAR_POSICAO_ORIGINAL || !mapaAtivo) return;

    *linhaOrig = totalLinhasOrig + 1;
    *colunaOrig = 1;
}

int preprocessar(FILE *entrada, FILE * saida){

    char linha[MAX_LINHA];
    int cols[MAX_LINHA];
    int gravadas = 0;
    int linhaOrig = 0;

    preLiberarMapa();

    while (fgets(linha, MAX_LINHA, entrada) != NULL){

        linhaOrig++;

        remover_quebra_linha(linha);
        remover_comentario(linha);
        normalizar_espacos_mapa(linha, cols);

        if(!linha_vazia(linha)){
            fprintf(saida, "%s\n", linha);
            mapaAdicionar(linhaOrig, linha, cols);
            gravadas++;
        }
    }

    totalLinhasOrig = linhaOrig;
    mapaAtivo = 1;

    return gravadas;
}

void PreProcessamento(FILE *entrada, FILE *saida){

    preprocessar(entrada, saida);

}