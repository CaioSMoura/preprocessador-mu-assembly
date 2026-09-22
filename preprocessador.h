#ifndef PREPROCESSADOR_H
#define PREPROCESSADOR_H

#include <stdio.h>

#define MAX_LINHA 1024
#define USAR_POSICAO_ORIGINAL 1

void remover_quebra_linha(char *linha);

void remover_comentario(char *linha);

void normalizar_espacos(char *linha);

void normalizar_espacos_mapa(char *linha, int *cols);

int linha_vazia(const char *linha);

int preprocessar(FILE *entrada, FILE *saida);

void PreProcessamento(FILE *entrada, FILE *saida);

void preMapear(int linhaPre, int colunaPre, int *linhaOrig, int *colunaOrig);

void preMapearEOF(int *linhaOrig, int *colunaOrig);

void preLiberarMapa(void);

#endif
