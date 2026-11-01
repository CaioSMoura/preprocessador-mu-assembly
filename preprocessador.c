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
static int ultimaSemQuebra = 0;  
void preLiberarMapa(void){
    for (int k = 0; k < mapaQtd; k++) {
        free(mapa[k].colOrig);
    }
    free(mapa);
    mapa = NULL;
    mapaQtd = mapaCap = 0;
    totalLinhasOrig = 0;
    mapaAtivo = 0;
    ultimaSemQuebra = 0;
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

int preLinhaTerminaNoEOF(int linhaPre){
    if (!mapaAtivo || linhaPre < 1 || linhaPre > mapaQtd) return 0;

    return ultimaSemQuebra && mapa[linhaPre - 1].linhaOrig == totalLinhasOrig;
}

static void semMemoria(void){
    fprintf(stderr, "ERRO: memoria insuficiente durante o pre-processamento.\n");
    exit(2);  
}

static long lerLinha(FILE *entrada, char **buf, size_t *cap, int *temQuebra){

    size_t n = 0;
    int leu = 0;
    int c;

    *temQuebra = 0;

    while ((c = fgetc(entrada)) != EOF) {

        leu = 1;

        if (c == '\n') {
            *temQuebra = 1;
            break;
        }

        if (c == '\r') {
            int prox = fgetc(entrada);
            if (prox != '\n' && prox != EOF) {
                ungetc(prox, entrada);
            }
            *temQuebra = 1;
            break;
        }

        if (n + 1 >= *cap) {
            size_t novaCap = (*cap == 0) ? 256 : *cap * 2;
            char *novo = realloc(*buf, novaCap);
            if (novo == NULL) semMemoria();
            *buf = novo;
            *cap = novaCap;
        }

        (*buf)[n++] = (char)c;
    }

    if (!leu) {
        return -1;
    }

    if (*cap == 0) {
        *buf = malloc(1);
        if (*buf == NULL) semMemoria();
        *cap = 1;
    }

    (*buf)[n] = '\0';
    return (long)n;
}

int preprocessar(FILE *entrada, FILE * saida){

    char *linha = NULL;
    size_t capLinha = 0;
    int *cols = NULL;
    size_t capCols = 0;
    int gravadas = 0;
    int linhaOrig = 0;
    int temQuebra = 1;

    preLiberarMapa();

    while (lerLinha(entrada, &linha, &capLinha, &temQuebra) >= 0){

        linhaOrig++;
        ultimaSemQuebra = !temQuebra;

        if (capCols < capLinha) {
            int *novo = realloc(cols, capLinha * sizeof(int));
            if (novo == NULL) semMemoria();
            cols = novo;
            capCols = capLinha;
        }

        remover_quebra_linha(linha);
        remover_comentario(linha);
        normalizar_espacos_mapa(linha, cols);

        if(!linha_vazia(linha)){
            fprintf(saida, "%s\n", linha);
            mapaAdicionar(linhaOrig, linha, cols);
            gravadas++;
        }
    }

    free(linha);
    free(cols);

    totalLinhasOrig = linhaOrig;
    mapaAtivo = 1;

    return gravadas;
}

void PreProcessamento(FILE *entrada, FILE *saida){

    preprocessar(entrada, saida);

}