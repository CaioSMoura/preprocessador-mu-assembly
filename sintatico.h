#ifndef SINTATICO_H
#define SINTATICO_H

#include <stdio.h>

#define MAX_CAT_SIN 50

/* Valores de retorno de AnaliseSintatica */
#define SIN_VALIDO    0
#define SIN_INVALIDO  1
#define SIN_FALHA    -1

typedef struct {
    char categoria[MAX_CAT_SIN];   /* ex.: "INS_LI", "REG", "EOL", "TK_EOF" */
    char *lexema;
    int linha;
    int coluna;
} TokenSin;

typedef struct No {
    int id;
    char *rotulo;                  /* "<programa>" ou "INS_LI\nli" */
    struct No **filhos;
    int qtdFilhos;
    int capFilhos;
} No;

typedef struct {
    TokenSin *toks;                /* tokens do .lex, com EOL e TK_EOF inseridos */
    int qtd;
    int cap;
    int pos;
    int qtdErros;
    FILE *erros;                   /* .sin.err */
    int proximoId;
} Parser;

int AnaliseSintatica(FILE *tokens, FILE *arvore, FILE *erros);
int carregarTokens(Parser *p, FILE *lex);
void liberarTokens(Parser *p);
TokenSin *atual(Parser *p);
TokenSin *consumir(Parser *p);
int verifica(Parser *p, const char *categoria);

/* ===== Pessoa 4: erros, recuperacao e arvore ===== */
No *noInterno(Parser *p, const char *nome);
No *noFolha(Parser *p, const TokenSin *t);
No *esperar(Parser *p, const char *categoria, const char *esperado);
void erroSintatico(Parser *p, const char *esperado);
void sincronizar(Parser *p);
void adicionarFilho(No *pai, No *filho);
void liberarArvore(No *n);
void gravarDot(FILE *out, No *raiz);

/* ===== Pessoa 2: programa, secoes, rotulos e diretivas ===== */
No *programa(Parser *p);

/* ===== Pessoa 3: instrucoes ===== */
No *instrucao(Parser *p);

#endif
