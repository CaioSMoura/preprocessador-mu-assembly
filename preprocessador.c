#include <stdio.h>
#include <string.h>
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
        if (linha[i] == '"') {
            dentro_string = !dentro_string;
        } else if (linha[i] == '#' && !dentro_string) {
            linha[i] = '\0';
            return;
        }
    } 
}

void normalizar_espacos(char *linha){
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
        // Alterna se entrar ou sair de uma string delimitada por aspas
        if (linha[i] == '"') {
            // Se tinha espaco acumulado fora da string, insere agora antes de abrir/fechar aspas
            if (espaco_pendente && j > 0) {
                linha[j++] = ' ';
                espaco_pendente = 0;
            }
            dentro_string = !dentro_string;
            linha[j++] = linha[i];
        } 
        else if (dentro_string) {
            // Se estiver dentro da string: copia exatamente tudo sem alterar (preserva tabs, espacos, etc.)
            linha[j++] = linha[i];
        } 
        else {
            // Fora da string: trata espacos e tabulacoes
            if (linha[i] == ' ' || linha[i] == '\t') {
                if (j > 0) {
                    espaco_pendente = 1; // Marca que ha espaco a ser inserido entre tokens
                }
            } else {
                // Caractere comum: escreve o espaco pendente (se houver) e depois o caractere
                if (espaco_pendente) {
                    linha[j++] = ' ';
                    espaco_pendente = 0;
                }
                linha[j++] = linha[i];
            }
        }
        i++;
    }

    // 3. Finaliza a string sem espacos no final (rtrim)
    linha[j] = '\0';
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

int preprocessar(FILE *entrada, FILE * saida){

    char linha[MAX_LINHA];
    int gravadas = 0;

    while (fgets(linha, MAX_LINHA, entrada) != NULL){
        
        remover_quebra_linha(linha);
        remover_comentario(linha);
        normalizar_espacos(linha);

        if(!linha_vazia(linha)){
            fprintf(saida, "%s\n", linha);
            gravadas++;
        }
    }
    
    return gravadas;
}

void PreProcessamento(FILE *entrada, FILE *saida){

    preprocessar(entrada, saida);

}