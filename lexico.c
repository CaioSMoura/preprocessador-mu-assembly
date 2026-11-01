#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "lexico.h"
#include "preprocessador.h"

static void semMemoria(void){
    fprintf(stderr, "ERRO: memoria insuficiente na analise lexica.\n");
    exit(2); 
}

static char *copiarTexto(const char *origem){
    size_t n = strlen(origem) + 1;
    char *copia = malloc(n);
    if (copia == NULL) semMemoria();
    memcpy(copia, origem, n);
    return copia;
}

typedef struct {
    char *dados;
    size_t tam;
    size_t cap;
} Texto;

static void textoIniciar(Texto *b){
    b->cap = 32;
    b->tam = 0;
    b->dados = malloc(b->cap);
    if (b->dados == NULL) semMemoria();
    b->dados[0] = '\0';
}

static void textoAdd(Texto *b, int c){
    if (b->tam + 1 >= b->cap) {
        size_t novaCap = b->cap * 2;
        char *novo = realloc(b->dados, novaCap);
        if (novo == NULL) semMemoria();
        b->dados = novo;
        b->cap = novaCap;
    }
    b->dados[b->tam++] = (char)c;
    b->dados[b->tam] = '\0';
}

static void textoLiberar(Texto *b){
    free(b->dados);
    b->dados = NULL;
    b->tam = b->cap = 0;
}

static int igualSemCaixa(const char *a, const char *b){
    while (*a != '\0' && *b != '\0') {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

// Copia origem -> destino em minusculas (no maximo tam-1 caracteres).
static void minusculas(const char *origem, char *destino, size_t tam){

    size_t i;

    for (i = 0; origem[i] != '\0' && i < tam - 1; i++) {

        destino[i] = (char)tolower((unsigned char)origem[i]);

    }

    destino[i] = '\0';

}


typedef struct {
    char tipo[MAX_CATEGORIA];
    char *lexema;
    int linha, coluna;
    int ordem;        
} ErroLexico;

static ErroLexico *listaErros = NULL;
static int qtdErros = 0;
static int capErros = 0;


int scannerLer(Scanner *s){
    int c;

    if (s->qtdDevolvidos > 0) {
        c = s->devolvidos[--s->qtdDevolvidos];
    } else {
        c = fgetc(s->arq);
    }

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

    if (s->qtdDevolvidos >= MAX_DEVOLVIDOS) {
        fprintf(stderr, "AVISO: lookahead excedido; caractere descartado.\n");
        return;
    }

    s->devolvidos[s->qtdDevolvidos++] = c;

    if (c == '\n'){

        s->linha--;

    } else if(c != '\r'){

        s->coluna--;

    }
}

static int seguidoDeDoisPontos(Scanner *s){
    int lidos[MAX_DEVOLVIDOS];
    int n = 0;
    int c = scannerLer(s);
    int achou;

    while ((c == ' ' || c == '\t') && n < MAX_DEVOLVIDOS - 1) {
        lidos[n++] = c;
        c = scannerLer(s);
    }

    achou = (c == ':');
    lidos[n++] = c;

    while (n > 0) {
        scannerDevolver(s, lidos[--n]);
    }

    return achou;
}

void montarToken(Token *t, const char *nome, const char *lexema, Scanner *s){

    strncpy(t->nome, nome, MAX_NOME - 1);
    t->nome[MAX_NOME - 1] = '\0';

    /* lexema[100] como no enunciado + copia completa (sem truncamento) */
    strncpy(t->lexema, lexema, MAX_LEXEMA - 1);
    t->lexema[MAX_LEXEMA - 1] = '\0';
    t->lexemaCompleto = copiarTexto(lexema);

    t->linha =  s->tokenLinha;
    t->coluna = s->tokenColuna;
}

void tokenLiberar(Token *t){
    free(t->lexemaCompleto);
    t->lexemaCompleto = NULL;
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

        preMapearEOF(&s->tokenLinha, &s->tokenColuna);

        montarToken(&t, "TK_EOF", "EOF", s);

        return t;

    }

    preMapear(s->linha, s->coluna, &s->tokenLinha, &s->tokenColuna);

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
    int fim;

    s.arq = in;
    s.linha = 1;
    s.coluna = 0;
    s.tokenLinha = 1;
    s.tokenColuna = 1;
    s.qtdDevolvidos = 0;

    tsInicializar();

    do {

        t = proximoToken(&s);
        fprintf(out, "<%s, %s> %d %d\n", t.nome, t.lexemaCompleto, t.linha, t.coluna);
        fim = (strcmp(t.nome, "TK_EOF") == 0);
        tokenLiberar(&t);

    } while (!fim);
}

void erroRegistrar(const char *tipo, const char *lexema, int linha, int coluna){

    if (qtdErros == capErros) {
        int novaCap = (capErros == 0) ? 32 : capErros * 2;
        ErroLexico *novo = realloc(listaErros, (size_t)novaCap * sizeof(ErroLexico));
        if (novo == NULL) semMemoria();
        listaErros = novo;
        capErros = novaCap;
    }

    ErroLexico *e = &listaErros[qtdErros];

    strncpy(e->tipo, tipo, MAX_CATEGORIA - 1);
    e->tipo[MAX_CATEGORIA - 1] = '\0';

    e->lexema = copiarTexto(lexema);
    e->linha = linha;
    e->coluna = coluna;
    e->ordem = qtdErros;

    qtdErros++;
}

static int compararErros(const void *a, const void *b){
    const ErroLexico *x = a;
    const ErroLexico *y = b;

    if (x->linha != y->linha)   return (x->linha < y->linha) ? -1 : 1;
    if (x->coluna != y->coluna) return (x->coluna < y->coluna) ? -1 : 1;
    return (x->ordem < y->ordem) ? -1 : (x->ordem > y->ordem);
}

void errosGravar(FILE *out){
    if (out == NULL) return;

    if (qtdErros == 0) {
        fprintf(out, "Nenhum erro léxico encontrado.\n");
        return;
    }

    qsort(listaErros, (size_t)qtdErros, sizeof(ErroLexico), compararErros);

    for (int i = 0; i < qtdErros; i++) {
        fprintf(out, "<%s, %s> %d %d\n",
                listaErros[i].tipo,
                listaErros[i].lexema,
                listaErros[i].linha,
                listaErros[i].coluna);
    }
}

static int delimitadorValido(int c){
    return (c == EOF) || (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r') ||
           (c == ',') || (c == ':') || (c == '(') || (c == ')') ||
           (c == '.') || (c == '$') || (c == '"');
}

static int categoriaReservada(const char *categoria);
static int tsConsultar(const char *lexema, char *categoria, char *nomeToken);
static int rejeitarRotuloReservado(Scanner *s, const char *chave, const char *lexema,
                                   Token *t){
    char categoria[MAX_CATEGORIA];

    if (!tsConsultar(chave, categoria, NULL) || !categoriaReservada(categoria)) {
        return 0;
    }

    if (!seguidoDeDoisPontos(s)) {
        return 0;
    }

    erroRegistrar("ERRO_ROTULO_RESERVADO", lexema, s->tokenLinha, s->tokenColuna);
    montarToken(t, "TK_ERRO", lexema, s);
    return 1;
}

void afdIdentificador(Scanner *s, int primeiro, Token *t){
    Texto lexema;
    textoIniciar(&lexema);

    textoAdd(&lexema, primeiro);
    int c = scannerLer(s);

    while (isalnum((unsigned char)c) || c == '_') {
        textoAdd(&lexema, c);
        c = scannerLer(s);
    }

    if (!delimitadorValido(c)) {
        while (!delimitadorValido(c)) {
            textoAdd(&lexema, c);
            c = scannerLer(s);
        }

        scannerDevolver(s, c);
        erroRegistrar("ERRO_IDENTIFICADOR_MALFORMADO", lexema.dados, s->tokenLinha, s->tokenColuna);
        montarToken(t, "TK_ERRO", lexema.dados, s);
        textoLiberar(&lexema);
        return;
    }

    scannerDevolver(s, c);

    if (rejeitarRotuloReservado(s, lexema.dados, lexema.dados, t)) {
        textoLiberar(&lexema);
        return;
    }

    char nomeToken[MAX_NOME];
    char categoria[MAX_CATEGORIA];

    if (tsBuscar(lexema.dados, categoria, nomeToken, s->tokenLinha, s->tokenColuna) == 1) {
        montarToken(t, nomeToken, lexema.dados, s);
    } else {
        tsInserir(lexema.dados, CAT_ID, s->tokenLinha, s->tokenColuna);
        montarToken(t, TK_ID, lexema.dados, s);
    }

    textoLiberar(&lexema);
}

void afdDiretiva(Scanner *s, int primeiro, Token *t){
    Texto lexema;
    textoIniciar(&lexema);

    textoAdd(&lexema, primeiro);
    int c = scannerLer(s);

    while (isalpha((unsigned char)c)) {
        textoAdd(&lexema, c);
        c = scannerLer(s);
    }

    if (isdigit((unsigned char)c) || c == '_') {
        while (isalnum((unsigned char)c) || c == '_') {
            textoAdd(&lexema, c);
            c = scannerLer(s);
        }
        scannerDevolver(s, c);
        erroRegistrar("ERRO_DIRETIVA_INVALIDA", lexema.dados, s->tokenLinha, s->tokenColuna);
        montarToken(t, "TK_ERRO", lexema.dados, s);
        textoLiberar(&lexema);
        return;
    }

    scannerDevolver(s, c);

    char nomeToken[MAX_NOME];
    char categoria[MAX_CATEGORIA];

    if (!tsConsultar(lexema.dados, categoria, NULL) || strcmp(categoria, CAT_DIRETIVA) != 0) {
        erroRegistrar("ERRO_DIRETIVA_INVALIDA", lexema.dados, s->tokenLinha, s->tokenColuna);
        montarToken(t, "TK_ERRO", lexema.dados, s);
    } else if (!rejeitarRotuloReservado(s, lexema.dados, lexema.dados, t)) {
        tsBuscar(lexema.dados, categoria, nomeToken, s->tokenLinha, s->tokenColuna);
        montarToken(t, nomeToken, lexema.dados, s);
    }

    textoLiberar(&lexema);
}

static int escapeValido(int c){
    return c == 'n' || c == 't' || c == '\\' || c == '"' || c == '0';
}

void afdString(Scanner *s, int primeiro, Token *t){
    Texto lexema;
    int escape_invalido = 0;

    textoIniciar(&lexema);
    textoAdd(&lexema, primeiro);

    int c = scannerLer(s);

    while (c != '"' && c != '\n' && c != EOF) {
        if (c == '\\') {
            int linhaEsc, colunaEsc;
            preMapear(s->linha, s->coluna, &linhaEsc, &colunaEsc);

            textoAdd(&lexema, c);
            c = scannerLer(s);

            if (c == '\n' || c == EOF) {
                break;
            }

            textoAdd(&lexema, c);

            if (!escapeValido(c)) {
                char seq[3];
                seq[0] = '\\';
                seq[1] = (char)c;
                seq[2] = '\0';
                erroRegistrar("ERRO_ESCAPE_INVALIDO", seq, linhaEsc, colunaEsc);
                escape_invalido++;
            }
        } else {
            textoAdd(&lexema, c);
        }
        c = scannerLer(s);
    }

    if (c == '\n' || c == EOF) {
        scannerDevolver(s, c);
        int noFimDoArquivo = (c == EOF) || preLinhaTerminaNoEOF(s->linha);

        erroRegistrar(noFimDoArquivo ? "ERRO_STRING_NAO_FECHADA_EOF"
                                     : "ERRO_STRING_NAO_FECHADA",
                      lexema.dados, s->tokenLinha, s->tokenColuna);
        montarToken(t, "TK_ERRO", lexema.dados, s);
    } else {
        textoAdd(&lexema, '"');
        montarToken(t, escape_invalido ? "TK_ERRO" : "STRING", lexema.dados, s);
    }

    textoLiberar(&lexema);
}


void afdRegistrador(Scanner *s, int primeiro, Token *t){
    Texto buffer;
    textoIniciar(&buffer);

    textoAdd(&buffer, primeiro);

    int c = scannerLer(s);
    while (isalnum((unsigned char)c) || c == '_') {
        textoAdd(&buffer, c);
        c = scannerLer(s);
    }
    scannerDevolver(s, c);

    char canonico[16];
    const char *chave = buffer.dados;
    const char *nomeReg = buffer.dados + 1;
    size_t qtdDigitos = strspn(nomeReg, "0123456789");

    if (qtdDigitos > 0 && nomeReg[qtdDigitos] == '\0') {
        while (*nomeReg == '0' && nomeReg[1] != '\0') {
            nomeReg++;                       /* descarta zeros a esquerda */
        }
        if (strlen(nomeReg) <= 2) {
            snprintf(canonico, sizeof canonico, "$%s", nomeReg);
            chave = canonico;
        }
    }

    char categoria[MAX_CATEGORIA];
    int valido = tsConsultar(chave, categoria, NULL) &&
                 strcmp(categoria, CAT_REGISTRADOR) == 0;

    if (!valido) {
        erroRegistrar("ERRO_REGISTRADOR_INVALIDO", buffer.dados, s->tokenLinha, s->tokenColuna);
        montarToken(t, "TK_ERRO", buffer.dados, s);
    } else if (!rejeitarRotuloReservado(s, chave, buffer.dados, t)) {
        tsBuscar(chave, NULL, NULL, s->tokenLinha, s->tokenColuna);
        montarToken(t, TK_REGISTRADOR, buffer.dados, s);
    }

    textoLiberar(&buffer);
}

static void numeroInvalido(Scanner *s, Texto *buffer, int c, const char *tipo, Token *t,
                           int aceitaPonto){
    while (isalnum((unsigned char)c) || c == '_' || (aceitaPonto && c == '.')) {
        textoAdd(buffer, c);
        c = scannerLer(s);
    }
    scannerDevolver(s, c);
    erroRegistrar(tipo, buffer->dados, s->tokenLinha, s->tokenColuna);
    montarToken(t, "TK_ERRO", buffer->dados, s);
}

void afdNumero(Scanner *s, int primeiro, Token *t){
    Texto buffer;
    textoIniciar(&buffer);

    textoAdd(&buffer, primeiro);

    /* Se comecou com '-', o proximo precisa ser digito */
    if (primeiro == '-') {
        int prox = scannerLer(s);
        if (!isdigit((unsigned char)prox)) {
            scannerDevolver(s, prox);
            textoLiberar(&buffer);
            afdSimbolo(s, primeiro, t);
            return;
        }
        textoAdd(&buffer, prox);
    }

    int c = scannerLer(s);

    int ehZero = (strcmp(buffer.dados, "0") == 0 || strcmp(buffer.dados, "-0") == 0);

    if (ehZero && (c == 'x' || c == 'X')) {
        textoAdd(&buffer, c);
        c = scannerLer(s);

        int temHex = 0;
        while (isxdigit((unsigned char)c)) {
            temHex = 1;
            textoAdd(&buffer, c);
            c = scannerLer(s);
        }

        if (!temHex || isalnum((unsigned char)c) || c == '_' || c == '.') {
            numeroInvalido(s, &buffer, c, "ERRO_NUMERO_INVALIDO", t, 1);
        } else {
            scannerDevolver(s, c);
            montarToken(t, "NUM_INT", buffer.dados, s);
        }
        textoLiberar(&buffer);
        return;
    }

    while (isdigit((unsigned char)c)) {
        textoAdd(&buffer, c);
        c = scannerLer(s);
    }

    if (c == '.') {
        numeroInvalido(s, &buffer, c, "ERRO_NUMERO_INVALIDO", t, 1);
    } else if (isalpha((unsigned char)c) || c == '_') {
        numeroInvalido(s, &buffer, c, "ERRO_IDENTIFICADOR_MALFORMADO", t, 0);
    } else {
        scannerDevolver(s, c);
        montarToken(t, "NUM_INT", buffer.dados, s);
    }

    textoLiberar(&buffer);
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

//   Palavras reservadas
static const char *const INSTRUCOES[] = {
    /* aritmetica */
    "add", "addu", "addi", "sub", "subu", "mul", "div",
    /* logicas */
    "and", "or", "xor", "nor",
    /* deslocamento */
    "sll", "srl",
    /* comparacao */
    "slt",
    /* memoria */
    "lw", "sw", "lb", "sb", "lh", "sh",
    /* carga de constantes / enderecos / copia */
    "li", "la", "move", "mfhi", "mflo",
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
    /* nomes simbolicos */
    "$zero", "$v0", "$v1",
    "$a0", "$a1", "$a2", "$a3",
    "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$t8", "$t9",
    "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
    "$k0", "$k1", "$gp", "$sp", "$fp", "$ra",
    /* forma numerica */
    "$0",  "$1",  "$2",  "$3",  "$4",  "$5",  "$6",  "$7",
    "$8",  "$9",  "$10", "$11", "$12", "$13", "$14", "$15",
    "$16", "$17", "$18", "$19", "$20", "$21", "$22", "$23",
    "$24", "$25", "$26", "$27", "$28", "$29", "$30", "$31"
};

static Simbolo *tabela = NULL;
static int total = 0;
static int capTabela = 0;
static int totalReservadas = 0;


static int categoriaReservada(const char *categoria){

    return strcmp(categoria, CAT_INSTRUCAO)   == 0 ||
           strcmp(categoria, CAT_DIRETIVA)    == 0 ||
           strcmp(categoria, CAT_REGISTRADOR) == 0;

}

static void nomeDoToken(const Simbolo *s, char *nomeToken){

    const char *p = s->lexemaCompleto;
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

static int indiceReservada(const char *lexema){

    for (int i = 0; i < totalReservadas; i++) {
        if (igualSemCaixa(tabela[i].lexemaCompleto, lexema)) {
            return i;
        }
    }

    return -1;
}

static int indiceExato(const char *lexema){

    for (int i = totalReservadas; i < total; i++) {
        if (strcmp(tabela[i].lexemaCompleto, lexema) == 0) {
            return i;
        }
    }

    return -1;
}

static int indiceDe(const char *lexema){
    int i = indiceReservada(lexema);
    return (i >= 0) ? i : indiceExato(lexema);
}

static void adicionar(const char *lexema, const char *categoria, int linha, int coluna){

    if (total == capTabela) {
        int novaCap = (capTabela == 0) ? 128 : capTabela * 2;
        Simbolo *novo = realloc(tabela, (size_t)novaCap * sizeof(Simbolo));
        if (novo == NULL) semMemoria();
        tabela = novo;
        capTabela = novaCap;
    }

    Simbolo *s = &tabela[total++];

    strncpy(s->lexema, lexema, MAX_LEXEMA - 1);
    s->lexema[MAX_LEXEMA - 1] = '\0';
    s->lexemaCompleto = copiarTexto(lexema);

    strncpy(s->categoria, categoria, MAX_CATEGORIA - 1);
    s->categoria[MAX_CATEGORIA - 1] = '\0';

    s->primeiraLinha = linha;
    s->primeiraColuna = coluna;
}

static void carregar(const char *const *lista, size_t n, const char *categoria){

    for (size_t i = 0; i < n; i++) {
        adicionar(lista[i], categoria, 0, 0);
    }
}

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

static void tsLiberar(void){
    for (int i = 0; i < total; i++) {
        free(tabela[i].lexemaCompleto);
    }
    free(tabela);
    tabela = NULL;
    total = capTabela = totalReservadas = 0;
}

static void errosLiberar(void){
    for (int i = 0; i < qtdErros; i++) {
        free(listaErros[i].lexema);
    }
    free(listaErros);
    listaErros = NULL;
    qtdErros = capErros = 0;
}

int lexicoQtdErros(void){
    return qtdErros;
}

void lexicoLiberar(void){
    tsLiberar();
    errosLiberar();
}

//   Interface publica (declarada em lexico.h)
void tsInicializar(void){

    tsLiberar();

    carregar(INSTRUCOES,    TAM(INSTRUCOES),    CAT_INSTRUCAO);
    carregar(DIRETIVAS,     TAM(DIRETIVAS),     CAT_DIRETIVA);
    carregar(REGISTRADORES, TAM(REGISTRADORES), CAT_REGISTRADOR);

    totalReservadas = total;
}

static int tsConsultar(const char *lexema, char *categoria, char *nomeToken){

    if (lexema == NULL) {
        return 0;
    }

    int i = indiceDe(lexema);

    if (i < 0) {
        return 0;
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
    - Palavras reservadas: comparacao sem distinguir maiusculas/minusculas.
    - Demais simbolos (rotulos): comparacao exata.
 */
int tsBuscar(const char *lexema, char *categoria, char *nomeToken, int linha, int coluna){

    if (!tsConsultar(lexema, categoria, nomeToken)) {
        return 0;
    }

    int i = indiceDe(lexema);

    if (tabela[i].primeiraLinha == 0 && tabela[i].primeiraColuna == 0) {
        tabela[i].primeiraLinha  = linha;
        tabela[i].primeiraColuna = coluna;
    }

    return 1;
}

/*
  - Insere um simbolo (normalmente CAT_ID) com a posicao da primeira ocorrencia.
  - Duplicatas sao bloqueadas: se o lexema ja existe, ou se colide com uma
    palavra reservada (ex.: um rotulo chamado "ADD"), nada e feito.
 */
void tsInserir(const char *lexema, const char *categoria, int linha, int coluna){

    if (lexema == NULL || categoria == NULL || lexema[0] == '\0') {
        return;
    }

    if (indiceReservada(lexema) >= 0) {
        return;
    }

    char *normalizado = copiarTexto(lexema);

    if (categoriaReservada(categoria)) {
        minusculas(lexema, normalizado, strlen(lexema) + 1);
    }

    if (indiceExato(normalizado) < 0) {
        adicionar(normalizado, categoria, linha, coluna);
    }

    free(normalizado);
}

// Grava a tabela inteira em CSV: LEXEMA,CATEGORIA,LINHA,COLUNA.
void tsGravar(FILE *out){

    fprintf(out, "LEXEMA,CATEGORIA,LINHA,COLUNA\n");

    for (int i = 0; i < total; i++) {
        gravarCampoCSV(out, tabela[i].lexemaCompleto);
        fputc(',', out);
        gravarCampoCSV(out, tabela[i].categoria);
        fprintf(out, ",%d,%d\n", tabela[i].primeiraLinha, tabela[i].primeiraColuna);
    }
}
