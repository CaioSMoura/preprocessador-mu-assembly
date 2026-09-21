#include <stdio.h>
#include <string.h>
#include <ctype.h> 
#include "lexico.h"

int scannerLer(Scanner *s){
    int c = fgetc(s->arq);

    if(c == '\n'){

        s->linha++;
        s->coluna = 0;

    }else if(c != '\r' && c != EOF) {

        s->coluna++;

    }

    return c;

}

void scannerDevolver(Scanner *s, int c){

    if (c == EOF ) {
        return;
    }

    ungetc(c, s-> arq);

    if (c == '\n'){
        
        s->linha--;

    } else if(c != '\r'){

        s->coluna--;

    }
}

void montarToken(Token *t, const char *nome, const char *lexema, Scanner *s){

    strncpy(t->nome, nome, MAX_NOME - 1);
    t->nome[MAX_NOME - 1] = '\0';

    strncpy(t->lexema, lexema, MAX_LEXEMA - 1);
    t->lexema[MAX_LEXEMA - 1] = '\0';
    
    t->linha =  s->tokenLinha;
    t->coluna = s->tokenColuna;
}

Token proximoToken(Scanner *s){

    Token t;
    int c;

    do {

        c = scannerLer(s);

    }while (c == ' ' || c == '\t' || c == '\n' || c == '\r');

    s->tokenLinha = s->linha;
    s->tokenColuna = s->coluna;

    if (c == EOF){

        montarToken(&t, "TK_EOF", "EOF", s);
        
        return t;

    }

    if(isalpha((unsigned char)c) || c == '_'){

        afdIdentificador(s, c, &t);

    } else if (c == '.') {

        afdDiretiva(s, c, &t);

    } else if (c == '"') {

        afdString(s, c, &t);

    } else if (c == '$'){

        afdRegistrador(s, c, &t);

    } else if (isdigit((unsigned char)c) || c == '-'){
               
        afdNumero(s, c, &t);

    } else {

        afdSimbolo(s, c, &t);

    }

    return t;

}

void AnaliseLexica(FILE *in, FILE *out){

    Scanner s; 
    Token t;

    s.arq = in;
    s.linha = 1;
    s.coluna = 0;
    s.tokenLinha = 1;
    s.tokenColuna = 1;

    tsInicializar();

    do {

        t = proximoToken(&s);
        fprintf(out, "<%s, %s> %d %d\n", t.nome, t.lexema, t.linha, t.coluna);

    } while (strcmp(t.nome, "TK_EOF") != 0);
}

void erroRegistrar(const char *tipo, const char *lexema, int linha, int coluna){
    if (qtdErros < MAX_ERROS) {
        strncpy(listaErros[qtdErros].tipo, tipo, MAX_CATEGORIA - 1);
        listaErros[qtdErros].tipo[MAX_CATEGORIA - 1] = '\0';

        strncpy(listaErros[qtdErros].lexema, lexema, MAX_LEXEMA - 1);
        listaErros[qtdErros].lexema[MAX_LEXEMA - 1] = '\0';

        listaErros[qtdErros].linha = linha;
        listaErros[qtdErros].coluna = coluna;
        qtdErros++;
    }

    (void)tipo; (void)lexema; (void)linha; (void)coluna;

}

void errosGravar(FILE *out){
    if (out == NULL) return;

    if (qtdErros == 0) {
        fprintf(out, "Nenhum erro lexico encontrado.\n");
    } else {
        for (int i = 0; i < qtdErros; i++) {
            fprintf(out, "Linha %d, Coluna %d: %s '%s'\n",
                    listaErros[i].linha,
                    listaErros[i].coluna,
                    listaErros[i].tipo,
                    listaErros[i].lexema);
        }
    }
}

/* 2. Sub-autómato de Registradores */
static const char *REGS_VALIDOS[] = {
    "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra",
    NULL
};

}

void afdIdentificador(Scanner *s, int primeiro, Token *t){

    char lex[2];

    lex[0] = (char)primeiro;
    lex[1] = '\0';

    montarToken(t, "TODO_ID", lex, s);

}

void afdDiretiva(Scanner *s, int primeiro, Token *t){

    char lex[2];

    lex[0] = (char)primeiro;
    lex[1] = '\0';

    montarToken(t, "TODO_DIR", lex, s);

}

void afdString(Scanner *s, int primeiro, Token *t){

    char lex[2];

    lex[0] = (char)primeiro;
    lex[1] = '\0';


    montarToken(t, "TODO_STRING", lex, s);

}

void afdRegistrador(Scanner *s, int primeiro, Token *t){
    char buffer[MAX_LEXEMA];
    int pos = 0;

    buffer[pos++] = (char)primeiro; // Armazena '$'

    int c = scannerLer(s);
    while (isalnum((unsigned char)c) || c == '_') {
        if (pos < MAX_LEXEMA - 1) {
            buffer[pos++] = (char)c;
        }
        c = scannerLer(s);
    }
    scannerDevolver(s, c);
    buffer[pos] = '\0';

    int valido = 0;
    char *nomeReg = buffer + 1; // Pula o '$'

    if (strlen(nomeReg) > 0) {
        // Verifica se é número entre 0 e 31
        char *endptr;
        long val = strtol(nomeReg, &endptr, 10);
        if (*endptr == '\0') {
            if (val >= 0 && val <= 31) {
                valido = 1;
            }
        } else {
            // Verifica mnemónico nominal válido
            for (int i = 0; REGS_VALIDOS[i] != NULL; i++) {
                if (strcmp(nomeReg, REGS_VALIDOS[i]) == 0) {
                    valido = 1;
                    break;
                }
            }
        }
    }

    if (valido) {
        montarToken(t, TK_REGISTRADOR, buffer, s);
    } else {
        erroRegistrar("ERRO_REGISTRADOR_INVALIDO", buffer, s->tokenLinha, s->tokenColuna);
        montarToken(t, "TK_ERRO", buffer, s);
    }
}


void afdNumero(Scanner *s, int primeiro, Token *t){
   char buffer[MAX_LEXEMA];
    int pos = 0;

    buffer[pos++] = (char)primeiro;

    // Se começou com '-', valida se o próximo é dígito imediato
    if (primeiro == '-') {
        int prox = scannerLer(s);
        if (!isdigit((unsigned char)prox)) {
            // Não é número: devolve e desvia para símbolo
            scannerDevolver(s, prox);
            afdSimbolo(s, primeiro, t);
            return;
        }
        buffer[pos++] = (char)prox;
    }

    int c = scannerLer(s);

    // Caso Hexadecimal (0x ou -0x)
    if ((primeiro == '0' && (c == 'x' || c == 'X')) ||
        (pos == 2 && buffer[1] == '0' && (c == 'x' || c == 'X'))) {
        buffer[pos++] = (char)c;
        c = scannerLer(s);

        int temHex = 0;
        while (isxdigit((unsigned char)c)) {
            temHex = 1;
            if (pos < MAX_LEXEMA - 1) buffer[pos++] = (char)c;
            c = scannerLer(s);
        }
        scannerDevolver(s, c);
        buffer[pos] = '\0';

        if (!temHex) {
            erroRegistrar("ERRO_NUMERO_INVALIDO", buffer, s->tokenLinha, s->tokenColuna);
            montarToken(t, "TK_ERRO", buffer, s);
            return;
        }

        montarToken(t, "TK_NUMERO", buffer, s);
        return;
    }

    // Caso Decimal
    while (isdigit((unsigned char)c)) {
        if (pos < MAX_LEXEMA - 1) buffer[pos++] = (char)c;
        c = scannerLer(s);
    }

    // Se tiver letras coladas nos dígitos (ex: 10abc) -> número mal formatado
    if (isalpha((unsigned char)c) || c == '_') {
        while (isalnum((unsigned char)c) || c == '_') {
            if (pos < MAX_LEXEMA - 1) buffer[pos++] = (char)c;
            c = scannerLer(s);
        }
        scannerDevolver(s, c);
        buffer[pos] = '\0';
        erroRegistrar("ERRO_NUMERO_INVALIDO", buffer, s->tokenLinha, s->tokenColuna);
        montarToken(t, "TK_ERRO", buffer, s);
        return;
    }

    scannerDevolver(s, c);
    buffer[pos] = '\0';
    montarToken(t, "TK_NUMERO", buffer, s);
} 


void afdSimbolo(Scanner *s, int primeiro, Token *t){
    char buffer[2];
    buffer[0] = (char)primeiro;
    buffer[1] = '\0';

    switch (primeiro) {
        case ',':
            montarToken(t, "TK_VIRGULA", buffer, s);
            break;
        case ':':
            montarToken(t, "TK_DOIS_PONTOS", buffer, s);
            break;
        case '(':
            montarToken(t, "TK_ABRE_PARENTESES", buffer, s);
            break;
        case ')':
            montarToken(t, "TK_FECHA_PARENTESES", buffer, s);
            break;
        default:
            erroRegistrar("ERRO_CARACTERE_INVALIDO", buffer, s->tokenLinha, s->tokenColuna);
            montarToken(t, "TK_ERRO", buffer, s);
            break;
    }

}