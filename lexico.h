#ifndef LEXICO_H
#define LEXICO_H

#include <stdio.h>

#define MAX_NOME 50
#define MAX_LEXEMA 100
#define MAX_CATEGORIA 30
#define MAX_SIMBOLOS 300
#define MAX_ERROS 500

/* Categorias da Tabela de Simbolos (campo "categoria" de Simbolo) */
#define CAT_INSTRUCAO   "INSTRUCAO"
#define CAT_DIRETIVA    "DIRETIVA"
#define CAT_REGISTRADOR "REGISTRADOR"
#define CAT_ID          "ID"

/* Nomes de token devolvidos por tsBuscar() em "nomeToken" */
#define TK_INSTRUCAO    "TK_INSTRUCAO"
#define TK_DIRETIVA     "TK_DIRETIVA"
#define TK_REGISTRADOR  "REG"
#define TK_ID           "ID"

typedef struct {

    char nome[MAX_NOME];
    char lexema[MAX_LEXEMA];

    int linha;
    int coluna;

} Token;

typedef struct {

    char lexema[MAX_LEXEMA];
    char categoria[MAX_CATEGORIA];

    int primeiraLinha;
    int primeiraColuna;

} Simbolo;

typedef struct {

    FILE *arq;

    int linha;
    int coluna;
    int tokenLinha;
    int tokenColuna;   

} Scanner;

int scannerLer(Scanner *s);

void scannerDevolver(Scanner *s, int c);

void montarToken(Token *t, const char *nome, const char *lexema, Scanner *s);

Token proximoToken(Scanner *s);

void AnaliseLexica(FILE *in, FILE *out);

void tsInicializar(void);
int tsBuscar(const char *lexema, char *categoria, char *nomeToken, int linha, int coluna);
void tsInserir(const char *lexema, const char *categoria, int linha, int coluna);
void tsGravar(FILE *out);

void erroRegistrar(const char *tipo, const char *lexema, int linha, int coluna);
void errosGravar(FILE *out);

void afdIdentificador(Scanner *s, int primeiro, Token *t);
void afdDiretiva(Scanner *s, int primeiro, Token *t);
void afdString(Scanner *s, int primeiro, Token *t);
void afdRegistrador(Scanner *s, int primeiro, Token *t);
void afdNumero(Scanner *s, int primeiro, Token *t);
void afdSimbolo(Scanner *s, int primeiro, Token *t);

#endif