#ifndef PREPROCESSADOR_H
#define PREPROCESSADOR_H

#include <stdio.h>

#define MAX_LINHA 1024

void remover_quebra_linha(char *linha);

void remover_comentario(char *linha);

void normalizar_espacos(char *linha);

int linha_vazia(const char *linha);

int preprocessar(FILE *entrada, FILE *saida);

void PreProcessamento(FILE *entrada, FILE *saida);

#endif
