#include <stdio.h>
#include <string.h>
#include <ctype.h> 
#include "lexico.h"
#include <stdlib.h>

typedef struct {
    char tipo[MAX_CATEGORIA];
    char lexema[MAX_LEXEMA];
    int linha, coluna;
} ErroLexico;

static ErroLexico listaErros[MAX_ERROS];
static int qtdErros = 0;

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

        if (s->tokenColuna == 0) {
            s->tokenColuna = 1;
        }

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
            fprintf(out, "<%s, %s> %d %d\n",
                    listaErros[i].tipo,
                    listaErros[i].lexema,
                    listaErros[i].linha,
                    listaErros[i].coluna);
        }
    }
}

void afdIdentificador(Scanner *s, int primeiro, Token *t){
    char lexema[MAX_LEXEMA];
    int i = 0;
    
    lexema[i++] = (char)primeiro;
    int c = scannerLer(s);
    
    while (isalnum((unsigned char)c) || c == '_') {
        if (i < MAX_LEXEMA - 1) lexema[i++] = (char)c;
        c = scannerLer(s);
    }

    int delimitadorValido =
        (c == EOF) || (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r') ||
        (c == ',') || (c == ':') || (c == '(') || (c == ')') ||
        (c == '.') || (c == '$') || (c == '"');
    
    if (!delimitadorValido) {
        /* Caractere colado sem separador -> identificador malformado */
        if (i < MAX_LEXEMA - 1) lexema[i++] = (char)c;
        lexema[i] = '\0';
        erroRegistrar("ERRO_IDENTIFICADOR_MALFORMADO", lexema, s->tokenLinha, s->tokenColuna);
        montarToken(t, "TK_ERRO", lexema, s);
        return;
    }

    scannerDevolver(s, c); 
    lexema[i] = '\0';

    char nomeToken[MAX_NOME];
    char categoria[MAX_CATEGORIA];
    strcpy(categoria, CAT_ID); 

   
    if (tsBuscar(lexema, categoria, nomeToken, s->tokenLinha, s->tokenColuna) == 1) {
       
        montarToken(t, nomeToken, lexema, s);
    } else {
       
        tsInserir(lexema, CAT_ID, s->tokenLinha, s->tokenColuna);
        montarToken(t, TK_ID, lexema, s);
    }
}

void afdDiretiva(Scanner *s, int primeiro, Token *t){
    char lexema[MAX_LEXEMA];
    int i = 0;
    
    lexema[i++] = (char)primeiro; 
    int c = scannerLer(s);
    
    while (isalpha((unsigned char)c)) {
        if (i < MAX_LEXEMA - 1) lexema[i++] = (char)c;
        c = scannerLer(s);
    }
    scannerDevolver(s, c);
    lexema[i] = '\0';

    char nomeToken[MAX_NOME];
    char categoria[MAX_CATEGORIA];
    strcpy(categoria, CAT_DIRETIVA);

    
    if (tsBuscar(lexema, categoria, nomeToken, s->tokenLinha, s->tokenColuna) == 1) {
        montarToken(t, nomeToken, lexema, s);
    } else {
        
        erroRegistrar("ERRO_DIRETIVA_INVALIDA", lexema, s->tokenLinha, s->tokenColuna);
        montarToken(t, "TK_ERRO", lexema, s);
    }
}

void afdString(Scanner *s, int primeiro, Token *t){
    char lexema[MAX_LEXEMA];
    int i = 0;
    int escape_invalido = 0;
    
    lexema[i++] = (char)primeiro; 
    int c = scannerLer(s);
    
    while (c != '"' && c != '\n' && c != EOF) {
        if (c == '\\') {
            if (i < MAX_LEXEMA - 1) lexema[i++] = (char)c;
            c = scannerLer(s);
            // Valida as 5 sequências de escape exigidas
            if (c == 'n' || c == 't' || c == '\\' || c == '"' || c == '0') {
                if (i < MAX_LEXEMA - 1) lexema[i++] = (char)c;
            } else {
                escape_invalido = 1;
                if (c != EOF && c != '\n' && i < MAX_LEXEMA - 1) {
                    lexema[i++] = (char)c;
                }
            }
        } else {
            if (i < MAX_LEXEMA - 1) lexema[i++] = (char)c;
        }
        c = scannerLer(s);
    }
    
   
    if (c == '\n' || c == EOF) {
        scannerDevolver(s, c);
        lexema[i] = '\0';
        erroRegistrar("ERRO_CADEIA_NAO_FECHADA", lexema, s->tokenLinha, s->tokenColuna);
        montarToken(t, "TK_ERRO", lexema, s);
    } 
    
    else if (escape_invalido) {
        if (i < MAX_LEXEMA - 1) lexema[i++] = '"';
        lexema[i] = '\0';
        erroRegistrar("ERRO_ESCAPE_INVALIDO", lexema, s->tokenLinha, s->tokenColuna);
        montarToken(t, "TK_ERRO", lexema, s);
    } 
    else { 
        if (i < MAX_LEXEMA - 1) lexema[i++] = '"';
        lexema[i] = '\0';
        montarToken(t, "STRING", lexema, s);
    }
}


void afdRegistrador(Scanner *s, int primeiro, Token *t){
    char buffer[MAX_LEXEMA];
    int pos = 0;

    buffer[pos++] = (char)primeiro;

    int c = scannerLer(s);
    while (isalnum((unsigned char)c) || c == '_') {
        if (pos < MAX_LEXEMA - 1) buffer[pos++] = (char)c;
        c = scannerLer(s);
    }
    scannerDevolver(s, c);
    buffer[pos] = '\0';

    int valido = 0;
    char *nomeReg = buffer + 1;

    if (strlen(nomeReg) > 0) {
        char *endptr;
        long val = strtol(nomeReg, &endptr, 10);
        if (*endptr == '\0') {
            valido = (val >= 0 && val <= 31);
        } else {
            char categoria[MAX_CATEGORIA];
            char nomeToken[MAX_NOME];
            valido = tsBuscar(buffer, categoria, nomeToken, s->tokenLinha, s->tokenColuna)
                     && strcmp(categoria, CAT_REGISTRADOR) == 0;
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

        montarToken(t, "NUM_INT", buffer, s);
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
        erroRegistrar("ERRO_IDENTIFICADOR_MALFORMADO", buffer, s->tokenLinha, s->tokenColuna);
        montarToken(t, "TK_ERRO", buffer, s);
        return;
    }

    scannerDevolver(s, c);
    buffer[pos] = '\0';
    montarToken(t, "NUM_INT", buffer, s);
} 


void afdSimbolo(Scanner *s, int primeiro, Token *t){
    char buffer[2];
    buffer[0] = (char)primeiro;
    buffer[1] = '\0';

    switch (primeiro) {
        case ',':
            montarToken(t, "SMB_COM", buffer, s);
            break;
        case ':':
            montarToken(t, "SMB_COL", buffer, s);
            break;
        case '(':
            montarToken(t, "SMB_OPA", buffer, s);
            break;
        case ')':
            montarToken(t, "SMB_CPA", buffer, s);
            break;
        default:
            erroRegistrar("ERRO_CARACTERE_INVALIDO", buffer, s->tokenLinha, s->tokenColuna);
            montarToken(t, "TK_ERRO", buffer, s);
            break;
    }

}

#define TAM(v) (sizeof(v) / sizeof((v)[0]))

/* --------------------------------------------------------------------------
 *  Palavras reservadas
 * -------------------------------------------------------------------------- */

static const char *const INSTRUCOES[] = {
    /* aritmetica */
    "add", "addu", "addi", "addiu", "sub", "subu", "mul", "div",
    /* logicas */
    "and", "andi", "or", "ori", "xor", "xori", "nor",
    /* deslocamento */
    "sll", "srl", "sra",
    /* comparacao */
    "slt", "slti", "sltu", "sltiu",
    /* memoria */
    "lw", "sw", "lb", "sb", "lh", "sh",
    /* carga de constantes / enderecos / copia */
    "lui", "li", "la", "move", "mfhi", "mflo",
    /* desvios e saltos */
    "beq", "bne", "blt", "bgt", "ble", "bge", "j", "jal", "jr",
    /* sistema */
    "syscall", "nop"
};

static const char *const DIRETIVAS[] = {
    ".data", ".text", ".globl", ".align", ".space",
    ".byte", ".half", ".word", ".ascii", ".asciiz"
};

static const char *const REGISTRADORES[] = {
    "$zero", "$v0", "$v1",
    "$a0", "$a1", "$a2", "$a3",
    "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$t8", "$t9",
    "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
    "$k0", "$k1", "$gp", "$sp", "$fp", "$ra"
};

/* --------------------------------------------------------------------------
 *  Estado da tabela
 * -------------------------------------------------------------------------- */

static Simbolo tabela[MAX_SIMBOLOS];
static int total = 0;
static int totalReservadas = 0;

/* --------------------------------------------------------------------------
 *  Auxiliares internas
 * -------------------------------------------------------------------------- */

/* Copia origem -> destino em minusculas (no maximo tam-1 caracteres). */
static void minusculas(const char *origem, char *destino, size_t tam){

    size_t i;

    for (i = 0; origem[i] != '\0' && i < tam - 1; i++) {

        destino[i] = (char)tolower((unsigned char)origem[i]);

    }

    destino[i] = '\0';

}

static int categoriaReservada(const char *categoria){

    return strcmp(categoria, CAT_INSTRUCAO)   == 0 ||
           strcmp(categoria, CAT_DIRETIVA)    == 0 ||
           strcmp(categoria, CAT_REGISTRADOR) == 0;

}

static void nomeDoToken(const Simbolo *s, char *nomeToken){

    const char *p = s->lexema;
    int j;

    if (strcmp(s->categoria, CAT_REGISTRADOR) == 0) {
        strcpy(nomeToken, "REG");
        return;
    }

    if (strcmp(s->categoria, CAT_INSTRUCAO) == 0) {
        strcpy(nomeToken, "INS_");
    } else if (strcmp(s->categoria, CAT_DIRETIVA) == 0) {
        strcpy(nomeToken, "DIR_");
        p++;                        
    } else {
        strcpy(nomeToken, "ID");
        return;
    }

    j = (int)strlen(nomeToken);

    for (; *p != '\0' && j < MAX_NOME - 1; p++) {
        nomeToken[j++] = (char)toupper((unsigned char)*p);
    }

    nomeToken[j] = '\0';
}

/* Procura nas palavras reservadas (o argumento ja deve estar em minusculas). */
static int indiceReservada(const char *minuscula){

    int i;

    for (i = 0; i < totalReservadas; i++) {

        if (strcmp(tabela[i].lexema, minuscula) == 0) {

            return i;

        }

    }

    return -1;

}

/* Procura correspondencia exata (case-sensitive) nos simbolos inseridos. */
static int indiceExato(const char *lexema){

    int i;

    for (i = totalReservadas; i < total; i++) {

        if (strcmp(tabela[i].lexema, lexema) == 0) {

            return i;

        }

    }

    return -1;

}

/* Acrescenta uma entrada ao fim da tabela. Retorna 1 se coube, 0 se cheia. */
static int adicionar(const char *lexema, const char *categoria, int linha, int coluna){

    Simbolo *s;

    if (total >= MAX_SIMBOLOS) {

        fprintf(stderr, "AVISO: tabela de simbolos cheia (%d). Ignorado: %s\n",
                MAX_SIMBOLOS, lexema);

        return 0;

    }

    s = &tabela[total++];

    strncpy(s->lexema, lexema, MAX_LEXEMA - 1);
    s->lexema[MAX_LEXEMA - 1] = '\0';

    strncpy(s->categoria, categoria, MAX_CATEGORIA - 1);
    s->categoria[MAX_CATEGORIA - 1] = '\0';

    s->primeiraLinha = linha;
    s->primeiraColuna = coluna;

    return 1;

}

static void carregar(const char *const *lista, size_t n, const char *categoria){

    size_t i;

    for (i = 0; i < n; i++) {

        adicionar(lista[i], categoria, 0, 0);

    }

}

/* Escreve um campo CSV, com aspas se tiver virgula, aspas ou quebra de linha. */
static void gravarCampoCSV(FILE *out, const char *campo){

    const char *p;
    int precisaAspas = (strpbrk(campo, ",\"\r\n") != NULL);

    if (!precisaAspas) {

        fputs(campo, out);
        return;

    }

    fputc('"', out);

    for (p = campo; *p != '\0'; p++) {

        if (*p == '"') {

            fputc('"', out);

        }

        fputc(*p, out);

    }

    fputc('"', out);

}

/* --------------------------------------------------------------------------
 *  Interface publica (declarada em lexico.h)
 * -------------------------------------------------------------------------- */

/* Zera a tabela e carrega as palavras reservadas. Pode ser chamada varias vezes. */
void tsInicializar(void){

    total = 0;

    carregar(INSTRUCOES,    TAM(INSTRUCOES),    CAT_INSTRUCAO);
    carregar(DIRETIVAS,     TAM(DIRETIVAS),     CAT_DIRETIVA);
    carregar(REGISTRADORES, TAM(REGISTRADORES), CAT_REGISTRADOR);

    totalReservadas = total;

}

/*
 * Consulta um lexema.
 *   - Palavras reservadas: comparacao sem distinguir maiusculas/minusculas.
 *   - Demais simbolos (rotulos): comparacao exata.
 *
 * Retorna 1 se encontrou, 0 caso contrario.
 * Se encontrou, preenche (quando nao forem NULL):
 *   categoria : "INSTRUCAO", "DIRETIVA", "REGISTRADOR" ou "ID"
 *               (buffer com pelo menos MAX_CATEGORIA bytes)
 *   nomeToken : "TK_INSTRUCAO", "DIR", "REG" ou "ID"
 *               (buffer com pelo menos MAX_NOME bytes)
 */
int tsBuscar(const char *lexema, char *categoria, char *nomeToken, int linha, int coluna){

    char minuscula[MAX_LEXEMA];
    int i;

    if (lexema == NULL) {
        return 0;
    }

    minusculas(lexema, minuscula, MAX_LEXEMA);

    i = indiceReservada(minuscula);

    if (i < 0) {
        i = indiceExato(lexema);
    }

    if (i < 0) {
        return 0;
    }

    if (tabela[i].primeiraLinha == 0 && tabela[i].primeiraColuna == 0) {
        tabela[i].primeiraLinha  = linha;
        tabela[i].primeiraColuna = coluna;
    }

    if (categoria != NULL) {
        snprintf(categoria, MAX_CATEGORIA, "%s", tabela[i].categoria);
    }

    if (nomeToken != NULL) {
        nomeDoToken(&tabela[i], nomeToken);
    }

    return 1;
}

/*
 * Insere um simbolo (normalmente CAT_ID) com a posicao da primeira ocorrencia.
 * Duplicatas sao bloqueadas: se o lexema ja existe, ou se colide com uma
 * palavra reservada (ex.: um rotulo chamado "ADD"), nada e feito.
 */
void tsInserir(const char *lexema, const char *categoria, int linha, int coluna){

    char minuscula[MAX_LEXEMA];
    char normalizado[MAX_LEXEMA];

    if (lexema == NULL || categoria == NULL || lexema[0] == '\0') {

        return;

    }

    minusculas(lexema, minuscula, MAX_LEXEMA);

    if (indiceReservada(minuscula) >= 0) {

        return;

    }

    /* Categorias reservadas sao normalizadas; rotulos preservam a caixa. */
    if (categoriaReservada(categoria)) {

        snprintf(normalizado, MAX_LEXEMA, "%s", minuscula);

    } else {

        snprintf(normalizado, MAX_LEXEMA, "%s", lexema);

    }

    if (indiceExato(normalizado) >= 0) {

        return;

    }

    adicionar(normalizado, categoria, linha, coluna);

}

/* Grava a tabela inteira em CSV: LEXEMA,CATEGORIA,LINHA,COLUNA. */
void tsGravar(FILE *out){

    int i;

    fprintf(out, "LEXEMA,CATEGORIA,LINHA,COLUNA\n");

    for (i = 0; i < total; i++) {

        gravarCampoCSV(out, tabela[i].lexema);
        fputc(',', out);
        gravarCampoCSV(out, tabela[i].categoria);
        fprintf(out, ",%d,%d\n", tabela[i].primeiraLinha, tabela[i].primeiraColuna);

    }

}