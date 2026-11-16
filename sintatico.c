#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sintatico.h"

static void semMemoria(void){
    fprintf(stderr, "ERRO: memoria insuficiente na analise sintatica.\n");
    exit(2); 
}

static char *copiarStr(const char *s){
    size_t n = strlen(s) + 1;
    char *c = malloc(n);
    if (c == NULL) semMemoria();
    memcpy(c, s, n);
    return c;
}

/* Le uma linha de tamanho arbitrario, sem o terminador. Retorna 0 no fim do arquivo. */
static int lerLinhaLex(FILE *f, char **buf, size_t *cap){
    size_t n = 0;
    int c;

    while ((c = fgetc(f)) != EOF && c != '\n') {
        if (n + 1 >= *cap) {
            size_t novaCap = (*cap == 0) ? 128 : *cap * 2;
            char *novo = realloc(*buf, novaCap);
            if (novo == NULL) semMemoria();
            *buf = novo;
            *cap = novaCap;
        }
        (*buf)[n++] = (char)c;
    }

    if (c == EOF && n == 0) {
        return 0;
    }

    if (*buf == NULL) {
        *buf = malloc(1);
        if (*buf == NULL) semMemoria();
        *cap = 1;
    }

    if (n > 0 && (*buf)[n - 1] == '\r') {
        n--;
    }
    (*buf)[n] = '\0';
    return 1;
}

static int lerInteiro(const char *s, int *valor){
    char *fim;
    long v;

    if (*s == '\0') return 0;
    v = strtol(s, &fim, 10);
    if (*fim != '\0') return 0;
    *valor = (int)v;
    return 1;
}

/* Separa uma linha "<CATEGORIA, lexema> linha coluna". */
static int separarLinha(char *s, char **cat, char **lex, int *linha, int *coluna){
    char *sep, *ult, *pen;

    if (s[0] != '<') return 0;

    sep = strstr(s + 1, ", ");
    if (sep == NULL) return 0;
    *sep = '\0';
    *cat = s + 1;
    *lex = sep + 2;

    ult = strrchr(*lex, ' ');
    if (ult == NULL || !lerInteiro(ult + 1, coluna)) return 0;
    *ult = '\0';

    pen = strrchr(*lex, ' ');
    if (pen == NULL || pen == *lex || pen[-1] != '>' || !lerInteiro(pen + 1, linha)) return 0;
    pen[-1] = '\0';

    return strlen(*cat) > 0 && strlen(*cat) < MAX_CAT_SIN;
}

static void adicionarToken(Parser *p, const char *cat, const char *lex, int linha, int coluna){
    TokenSin *t;

    if (p->qtd == p->cap) {
        int novaCap = (p->cap == 0) ? 128 : p->cap * 2;
        TokenSin *novo = realloc(p->toks, (size_t)novaCap * sizeof(TokenSin));
        if (novo == NULL) semMemoria();
        p->toks = novo;
        p->cap = novaCap;
    }

    t = &p->toks[p->qtd++];
    snprintf(t->categoria, MAX_CAT_SIN, "%s", cat);
    t->lexema = copiarStr(lex);
    t->linha = linha;
    t->coluna = coluna;
}

/* Insere EOL logo apos o ultimo token, se a linha dele ainda nao foi encerrada. */
static void encerrarLinha(Parser *p){
    TokenSin *ult;

    if (p->qtd == 0) return;

    ult = &p->toks[p->qtd - 1];
    if (strcmp(ult->categoria, "EOL") == 0) return;

    adicionarToken(p, "EOL", "", ult->linha, ult->coluna + (int)strlen(ult->lexema));
}

int carregarTokens(Parser *p, FILE *lex){
    char *linhaTxt = NULL;
    size_t cap = 0;
    int numLinha = 0;
    int temEOF = 0;

    rewind(lex);

    while (!temEOF && lerLinhaLex(lex, &linhaTxt, &cap)) {
        char *cat, *lexema;
        int linha, coluna;

        numLinha++;

        if (linhaTxt[0] == '\0') continue;

        if (!separarLinha(linhaTxt, &cat, &lexema, &linha, &coluna)) {
            fprintf(stderr, "ERRO: linha %d do arquivo de tokens esta mal formada.\n", numLinha);
            free(linhaTxt);
            return -1;
        }

        if (strcmp(cat, "TK_EOF") == 0) {
            encerrarLinha(p);
            adicionarToken(p, "TK_EOF", lexema, linha, coluna);
            temEOF = 1;
            continue;
        }

        if (p->qtd > 0 && p->toks[p->qtd - 1].linha != linha) {
            encerrarLinha(p);
        }

        adicionarToken(p, cat, lexema, linha, coluna);
    }

    free(linhaTxt);

    if (ferror(lex)) {
        fprintf(stderr, "ERRO: falha na leitura do arquivo de tokens.\n");
        return -1;
    }

    /* .lex sem TK_EOF explicito: acrescenta um, sem duplicar */
    if (!temEOF) {
        int linha = (p->qtd > 0) ? p->toks[p->qtd - 1].linha + 1 : 1;
        encerrarLinha(p);
        adicionarToken(p, "TK_EOF", "EOF", linha, 1);
    }

    return 0;
}

void liberarTokens(Parser *p){
    for (int i = 0; i < p->qtd; i++) {
        free(p->toks[i].lexema);
    }
    free(p->toks);
    p->toks = NULL;
    p->qtd = p->cap = p->pos = 0;
}

TokenSin *atual(Parser *p){
    return &p->toks[p->pos];
}

/* Avanca para o proximo token; nunca passa de TK_EOF. */
TokenSin *consumir(Parser *p){
    TokenSin *t = atual(p);

    if (strcmp(t->categoria, "TK_EOF") != 0) {
        p->pos++;
    }
    return t;
}

int verifica(Parser *p, const char *categoria){
    return strcmp(atual(p)->categoria, categoria) == 0;
}

int AnaliseSintatica(FILE *tokens, FILE *arvore, FILE *erros){
    Parser p;
    No *raiz;
    int resultado;

    if (tokens == NULL || arvore == NULL || erros == NULL) {
        return SIN_FALHA;
    }

    memset(&p, 0, sizeof p);
    p.erros = erros;

    if (carregarTokens(&p, tokens) != 0) {
        liberarTokens(&p);
        return SIN_FALHA;
    }

    raiz = programa(&p);

    resultado = (p.qtdErros == 0 && raiz != NULL) ? SIN_VALIDO : SIN_INVALIDO;

    /* Programa invalido: o .dot fica vazio */
    if (resultado == SIN_VALIDO) {
        gravarDot(arvore, raiz);
        fprintf(erros, "Nenhum erro sintático encontrado.\n");
    }

    liberarArvore(raiz);
    liberarTokens(&p);

    if (ferror(arvore) || ferror(erros)) {
        return SIN_FALHA;
    }

    return resultado;
}


/* ===== Pessoa 4: erros, recuperacao e arvore ===== */

No *noInterno(Parser *p, const char *nome){
    No *n = calloc(1, sizeof(No));
    if (n == NULL) semMemoria();
    n->id = p->proximoId++;
    n->rotulo = copiarStr(nome);
    return n;
}

No *noFolha(Parser *p, const TokenSin *t){
    No *n;
    size_t tam = strlen(t->categoria) + strlen(t->lexema) + 4;
    char *rotulo = malloc(tam);

    if (rotulo == NULL) semMemoria();

    if (t->lexema[0] == '\0' || strcmp(t->categoria, "EOL") == 0 || strcmp(t->categoria, "TK_EOF") == 0) {
        snprintf(rotulo, tam, "%s", t->categoria);
    } else {
        snprintf(rotulo, tam, "%s\n%s", t->categoria, t->lexema);
    }

    n = noInterno(p, rotulo);
    free(rotulo);
    return n;
}

void erroSintatico(Parser *p, const char *esperado){
    TokenSin *t = atual(p);

    p->qtdErros++;
    if (p->erros == NULL || t == NULL) return;

    fprintf(p->erros, "Linha %d, coluna %d:\n", t->linha, t->coluna);

    if (strcmp(t->categoria, "EOL") == 0) {
        fprintf(p->erros, "Encontrado fim de linha (EOL); esperado %s.\n", esperado);
    } else if (strcmp(t->categoria, "TK_EOF") == 0) {
        fprintf(p->erros, "Encontrado TK_EOF; esperado %s.\n", esperado);
    } else {
        fprintf(p->erros, "Encontrado %s(\"%s\"); esperado %s.\n",
                t->categoria, t->lexema, esperado);
    }
}

No *esperar(Parser *p, const char *categoria, const char *esperado){
    if (!verifica(p, categoria)) {
        erroSintatico(p, esperado);
        return NULL;
    }
    return noFolha(p, consumir(p));
}

void sincronizar(Parser *p){
    while (!verifica(p, "EOL") && !verifica(p, "TK_EOF")) {
        consumir(p);
    }
    if (verifica(p, "EOL")) {
        consumir(p);
    }
}

void adicionarFilho(No *pai, No *filho){
    if (pai == NULL || filho == NULL) return;

    if (pai->qtdFilhos == pai->capFilhos) {
        int novaCap = (pai->capFilhos == 0) ? 4 : pai->capFilhos * 2;
        No **novo = realloc(pai->filhos, (size_t)novaCap * sizeof(No *));
        if (novo == NULL) semMemoria();
        pai->filhos = novo;
        pai->capFilhos = novaCap;
    }
    pai->filhos[pai->qtdFilhos++] = filho;
}

void liberarArvore(No *n){
    if (n == NULL) return;
    for (int i = 0; i < n->qtdFilhos; i++) {
        liberarArvore(n->filhos[i]);
    }
    free(n->filhos);
    free(n->rotulo);
    free(n);
}

static void gravarRotulo(FILE *out, const char *s){
    for (; *s != '\0'; s++) {
        if (*s == '"' || *s == '\\') {
            fputc('\\', out);
            fputc(*s, out);
        } else if (*s == '\n') {
            fputs("\\n", out);
        } else {
            fputc(*s, out);
        }
    }
}

static void gravarNos(FILE *out, No *n){
    if (n == NULL) return;
    fprintf(out, "  n%d [label=\"", n->id);
    gravarRotulo(out, n->rotulo);
    fprintf(out, "\"];\n");
    for (int i = 0; i < n->qtdFilhos; i++) {
        gravarNos(out, n->filhos[i]);
    }
}

static void gravarArestas(FILE *out, No *n){
    if (n == NULL) return;
    for (int i = 0; i < n->qtdFilhos; i++) {
        if (n->filhos[i] != NULL) {
            fprintf(out, "  n%d -> n%d;\n", n->id, n->filhos[i]->id);
            gravarArestas(out, n->filhos[i]);
        }
    }
}

void gravarDot(FILE *out, No *raiz){
    if (out == NULL) return;
    fprintf(out, "digraph ArvoreSintatica {\n");
    fprintf(out, "  graph [ordering=out];\n");
    fprintf(out, "  node [shape=box];\n");
    fprintf(out, "  edge [arrowhead=none];\n");
    if (raiz != NULL) {
        gravarNos(out, raiz);
        gravarArestas(out, raiz);
    }
    fprintf(out, "}\n");
}

/* ===== Pessoa 2: programa, secoes, rotulos e diretivas ===== */

/* Categorias do léxico usadas neste bloco.
   Se o .lex usar outro nome, é só ajustar aqui. */
#define CAT_DIR_DATA    "DIR_DATA"
#define CAT_DIR_TEXT    "DIR_TEXT"
#define CAT_DIR_GLOBL   "DIR_GLOBL"
#define CAT_DIR_WORD    "DIR_WORD"
#define CAT_DIR_HALF    "DIR_HALF"
#define CAT_DIR_BYTE    "DIR_BYTE"
#define CAT_DIR_ASCII   "DIR_ASCII"
#define CAT_DIR_ASCIIZ  "DIR_ASCIIZ"
#define CAT_DIR_SPACE   "DIR_SPACE"
#define CAT_DIR_ALIGN   "DIR_ALIGN"
#define CAT_ID          "ID"
#define CAT_NUM_INT     "NUM_INT"
#define CAT_STRING      "STRING"
#define CAT_VIRGULA     "SMB_COM"
#define CAT_DOIS_PONTOS "SMB_COL"
#define CAT_EOL         "EOL"
#define CAT_EOF         "TK_EOF"

/* ---------- auxiliares (só deste bloco) ---------- */

/* Anexa o filho se ele existir. Devolve 1 se anexou, 0 se filho era NULL. */
static int anexar(No *pai, No *filho) {
    if (!filho) return 0;
    adicionarFilho(pai, filho);
    return 1;
}

/* Libera um nó parcial (com os filhos já anexados) e devolve NULL. */
static No *falha(No *n) {
    liberarArvore(n);
    return NULL;
}

/* Consome o token atual, seja qual for a categoria (já verificada antes). */
static No *consumirAtual(Parser *p) {
    return esperar(p, atual(p)->categoria, atual(p)->categoria);
}

static int ehInstrucao(Parser *p) {
    TokenSin *t = atual(p);
    return t && strncmp(t->categoria, "INS_", 4) == 0;
}

static int ehDiretivaInteiros(Parser *p) {
    return verifica(p, CAT_DIR_WORD) || verifica(p, CAT_DIR_HALF) ||
           verifica(p, CAT_DIR_BYTE) || verifica(p, CAT_DIR_SPACE) ||
           verifica(p, CAT_DIR_ALIGN);
}

static int ehDiretivaString(Parser *p) {
    return verifica(p, CAT_DIR_ASCII) || verifica(p, CAT_DIR_ASCIIZ);
}

/* As repetições de linha só param em DIR_DATA, DIR_TEXT ou TK_EOF. */
static int fimDeSecao(Parser *p) {
    return verifica(p, CAT_DIR_DATA) || verifica(p, CAT_DIR_TEXT) ||
           verifica(p, CAT_EOF);
}

/* ---------- diretivas ---------- */

/* <lista_inteiros> ::= NUM_INT { SMB_COM NUM_INT }
   Lista terminada em vírgula gera erro no NUM_INT que falta. */
No *lista_inteiros(Parser *p) {
    No *n = noInterno(p, "<lista_inteiros>");
    if (!anexar(n, esperar(p, CAT_NUM_INT, "NUM_INT"))) return falha(n);
    while (verifica(p, CAT_VIRGULA)) {
        anexar(n, consumirAtual(p));
        if (!anexar(n, esperar(p, CAT_NUM_INT, "NUM_INT"))) return falha(n);
    }
    return n;
}

/* <diretiva_inteiros> ::= (DIR_WORD | DIR_HALF | DIR_BYTE) <lista_inteiros>
                         | (DIR_SPACE | DIR_ALIGN) NUM_INT */
No *diretiva_inteiros(Parser *p) {
    No *n = noInterno(p, "<diretiva_inteiros>");
    int comLista = verifica(p, CAT_DIR_WORD) || verifica(p, CAT_DIR_HALF) ||
                   verifica(p, CAT_DIR_BYTE);
    if (!anexar(n, consumirAtual(p))) return falha(n);
    if (comLista) {
        if (!anexar(n, lista_inteiros(p))) return falha(n);
    } else if (!anexar(n, esperar(p, CAT_NUM_INT, "NUM_INT"))) {
        return falha(n);
    }
    return n;
}

/* <diretiva_string> ::= (DIR_ASCII | DIR_ASCIIZ) STRING */
No *diretiva_string(Parser *p) {
    No *n = noInterno(p, "<diretiva_string>");
    if (!anexar(n, consumirAtual(p))) return falha(n);
    if (!anexar(n, esperar(p, CAT_STRING, "STRING"))) return falha(n);
    return n;
}

/* <diretiva_dados> ::= <diretiva_inteiros> | <diretiva_string> */
No *diretiva_dados(Parser *p) {
    No *n = noInterno(p, "<diretiva_dados>");
    No *f = ehDiretivaString(p) ? diretiva_string(p) : diretiva_inteiros(p);
    if (!anexar(n, f)) return falha(n);
    return n;
}

/* <diretiva_global> ::= DIR_GLOBL ID   (vale em .data e em .text) */
No *diretiva_global(Parser *p) {
    No *n = noInterno(p, "<diretiva_global>");
    if (!anexar(n, esperar(p, CAT_DIR_GLOBL, "DIR_GLOBL"))) return falha(n);
    if (!anexar(n, esperar(p, CAT_ID, "ID"))) return falha(n);
    return n;
}

/* ---------- rótulo ---------- */

/* <rotulo> ::= ID SMB_COL   (rótulo sem ':' gera erro no ':') */
No *rotulo(Parser *p) {
    No *n = noInterno(p, "<rotulo>");
    if (!anexar(n, esperar(p, CAT_ID, "ID"))) return falha(n);
    if (!anexar(n, esperar(p, CAT_DOIS_PONTOS, "SMB_COL(\":\")"))) return falha(n);
    return n;
}

/* ---------- linhas ---------- */

/* <linha_dados> ::= [<rotulo>] [<diretiva_global> | <diretiva_dados>] EOL
   Em caso de erro: libera o parcial, sincroniza e devolve NULL. */
No *linha_dados(Parser *p) {
    No *n = noInterno(p, "<linha_dados>");
    int temRotulo = 0;

    if (verifica(p, CAT_ID)) {
        if (!anexar(n, rotulo(p))) goto erro;
        temRotulo = 1;
    }
    if (verifica(p, CAT_DIR_GLOBL)) {
        if (!anexar(n, diretiva_global(p))) goto erro;
    } else if (ehDiretivaInteiros(p) || ehDiretivaString(p)) {
        if (!anexar(n, diretiva_dados(p))) goto erro;
    } else if (!verifica(p, CAT_EOL)) {
        erroSintatico(p, temRotulo ? "diretiva de dados ou EOL"
                                   : "rotulo ou diretiva de dados");
        goto erro;
    }
    if (!anexar(n, esperar(p, CAT_EOL, "EOL"))) goto erro;
    return n;

erro:
    liberarArvore(n);
    sincronizar(p);
    return NULL;
}

/* <linha_codigo> ::= [<rotulo>] [<diretiva_global> | <instrucao>] EOL
   A instrução (Pessoa 3) termina ANTES do EOL; quem consome o EOL é esta função. */
No *linha_codigo(Parser *p) {
    No *n = noInterno(p, "<linha_codigo>");
    int temRotulo = 0;

    if (verifica(p, CAT_ID)) {
        if (!anexar(n, rotulo(p))) goto erro;
        temRotulo = 1;
    }
    if (verifica(p, CAT_DIR_GLOBL)) {
        if (!anexar(n, diretiva_global(p))) goto erro;
    } else if (ehInstrucao(p)) {
        if (!anexar(n, instrucao(p))) goto erro;
    } else if (!verifica(p, CAT_EOL)) {
        erroSintatico(p, temRotulo ? "instrucao ou EOL"
                                   : "rotulo ou instrucao");
        goto erro;
    }
    if (!anexar(n, esperar(p, CAT_EOL, "EOL"))) goto erro;
    return n;

erro:
    liberarArvore(n);
    sincronizar(p);
    return NULL;
}

/* ---------- seções ---------- */

/* <secao_dados> ::= DIR_DATA EOL { <linha_dados> } */
No *secao_dados(Parser *p) {
    No *n = noInterno(p, "<secao_dados>");
    anexar(n, esperar(p, CAT_DIR_DATA, "DIR_DATA"));
    if (!anexar(n, esperar(p, CAT_EOL, "EOL"))) sincronizar(p);
    while (!fimDeSecao(p)) {
        int antes = p->pos;
        anexar(n, linha_dados(p));
        if (p->pos == antes) p->pos++;   /* segurança: sempre avança */
    }
    return n;
}

/* <secao_codigo> ::= DIR_TEXT EOL { <linha_codigo> } */
No *secao_codigo(Parser *p) {
    No *n = noInterno(p, "<secao_codigo>");
    anexar(n, esperar(p, CAT_DIR_TEXT, "DIR_TEXT"));
    if (!anexar(n, esperar(p, CAT_EOL, "EOL"))) sincronizar(p);
    while (!fimDeSecao(p)) {
        int antes = p->pos;
        anexar(n, linha_codigo(p));
        if (p->pos == antes) p->pos++;   /* segurança: sempre avança */
    }
    return n;
}

/* <secao> ::= <secao_dados> | <secao_codigo> */
No *secao(Parser *p) {
    No *n = noInterno(p, "<secao>");
    No *f;
    if (verifica(p, CAT_DIR_DATA))      f = secao_dados(p);
    else if (verifica(p, CAT_DIR_TEXT)) f = secao_codigo(p);
    else {
        erroSintatico(p, "DIR_DATA(\".data\") ou DIR_TEXT(\".text\")");
        liberarArvore(n);
        sincronizar(p);
        return NULL;
    }
    if (!anexar(n, f)) return falha(n);
    return n;
}

/* <programa> ::= <secao> { <secao> } TK_EOF
   Sempre devolve a raiz; quem decide se há erro é p->qtdErros. */
No *programa(Parser *p) {
    No *raiz = noInterno(p, "<programa>");
    int secoes = 0;
    while (!verifica(p, CAT_EOF)) {
        if (anexar(raiz, secao(p))) secoes++;
    }
    if (secoes == 0 && p->qtdErros == 0)      /* arquivo sem nenhuma seção */
        erroSintatico(p, "DIR_DATA(\".data\") ou DIR_TEXT(\".text\")");
    anexar(raiz, esperar(p, CAT_EOF, "TK_EOF"));
    return raiz;
}
/* ===== fim do bloco da Pessoa 2 ===== */

/* ===== Pessoa 3(VERSAO PROVISORIA): instrucoes ===== */

#define CAT_REG    "REG"
#define CAT_ABRE   "SMB_OPA"
#define CAT_FECHA  "SMB_CPA"

#define ESP_REG    "REG"
#define ESP_VIRG   "SMB_COM(\",\")"
#define ESP_ABRE   "SMB_OPA(\"(\")"
#define ESP_FECHA  "SMB_CPA(\")\")"
#define ESP_NUM    "NUM_INT"
#define ESP_ID     "ID"

/* Grupos de instrucao (um por funcao instrucao_*) */
enum {
    GRP_NENHUM = 0, GRP_TRES_REG, GRP_DOIS_REG, GRP_UM_REG, GRP_IMEDIATO,
    GRP_DESLOCAMENTO, GRP_LI, GRP_LA, GRP_MEMORIA, GRP_DESVIO, GRP_SALTO,
    GRP_SEM_OPERANDOS
};

typedef struct { const char *categoria; int grupo; } MnemonicoGrupo;

/* Os mnemonicos reconhecidos pelo lexico (36 no total, conferir com INSTRUCOES[] em lexico.c). */
static const MnemonicoGrupo MNEMONICOS[] = {
    /* REG , REG , REG */
    {"INS_ADD", GRP_TRES_REG}, {"INS_ADDU", GRP_TRES_REG}, {"INS_SUB", GRP_TRES_REG},
    {"INS_SUBU", GRP_TRES_REG}, {"INS_MUL", GRP_TRES_REG}, {"INS_AND", GRP_TRES_REG},
    {"INS_OR", GRP_TRES_REG}, {"INS_XOR", GRP_TRES_REG}, {"INS_NOR", GRP_TRES_REG},
    {"INS_SLT", GRP_TRES_REG},
    /* REG , REG */
    {"INS_DIV", GRP_DOIS_REG}, {"INS_MOVE", GRP_DOIS_REG},
    /* REG */
    {"INS_MFHI", GRP_UM_REG}, {"INS_MFLO", GRP_UM_REG}, {"INS_JR", GRP_UM_REG},
    /* REG , REG , NUM_INT */
    {"INS_ADDI", GRP_IMEDIATO},
    {"INS_SLL", GRP_DESLOCAMENTO}, {"INS_SRL", GRP_DESLOCAMENTO},
    /* REG , NUM_INT  /  REG , ID */
    {"INS_LI", GRP_LI}, {"INS_LA", GRP_LA},
    /* REG , endereco */
    {"INS_LW", GRP_MEMORIA}, {"INS_SW", GRP_MEMORIA}, {"INS_LB", GRP_MEMORIA},
    {"INS_SB", GRP_MEMORIA}, {"INS_LH", GRP_MEMORIA}, {"INS_SH", GRP_MEMORIA},
    /* REG , REG , ID */
    {"INS_BEQ", GRP_DESVIO}, {"INS_BNE", GRP_DESVIO}, {"INS_BLT", GRP_DESVIO},
    {"INS_BGT", GRP_DESVIO}, {"INS_BLE", GRP_DESVIO}, {"INS_BGE", GRP_DESVIO},
    /* ID */
    {"INS_J", GRP_SALTO}, {"INS_JAL", GRP_SALTO},
    /* sem operandos */
    {"INS_SYSCALL", GRP_SEM_OPERANDOS}, {"INS_NOP", GRP_SEM_OPERANDOS}
};

static int grupoDoMnemonico(const char *categoria){
    for (size_t i = 0; i < sizeof MNEMONICOS / sizeof MNEMONICOS[0]; i++) {
        if (strcmp(MNEMONICOS[i].categoria, categoria) == 0) return MNEMONICOS[i].grupo;
    }
    return GRP_NENHUM;
}

/* Anexa "SMB_COM REG" ao pai: "," seguida de registrador. */
static int virgulaEReg(Parser *p, No *pai){
    return anexar(pai, esperar(p, CAT_VIRGULA, ESP_VIRG)) &&
           anexar(pai, esperar(p, CAT_REG, ESP_REG));
}

/* ---------- operandos ---------- */

/* <op_tres_registradores> ::= REG SMB_COM REG SMB_COM REG */
static No *op_tres_registradores(Parser *p){
    No *n = noInterno(p, "<op_tres_registradores>");
    if (!anexar(n, esperar(p, CAT_REG, ESP_REG))) return falha(n);
    if (!virgulaEReg(p, n)) return falha(n);
    if (!virgulaEReg(p, n)) return falha(n);
    return n;
}

/* <op_dois_registradores> ::= REG SMB_COM REG */
static No *op_dois_registradores(Parser *p){
    No *n = noInterno(p, "<op_dois_registradores>");
    if (!anexar(n, esperar(p, CAT_REG, ESP_REG))) return falha(n);
    if (!virgulaEReg(p, n)) return falha(n);
    return n;
}

/* <op_um_registrador> ::= REG */
static No *op_um_registrador(Parser *p){
    No *n = noInterno(p, "<op_um_registrador>");
    if (!anexar(n, esperar(p, CAT_REG, ESP_REG))) return falha(n);
    return n;
}

/* <op_deslocamento> ::= REG SMB_COM REG SMB_COM NUM_INT */
static No *op_deslocamento(Parser *p){
    No *n = noInterno(p, "<op_deslocamento>");
    if (!anexar(n, esperar(p, CAT_REG, ESP_REG))) return falha(n);
    if (!virgulaEReg(p, n)) return falha(n);
    if (!anexar(n, esperar(p, CAT_VIRGULA, ESP_VIRG))) return falha(n);
    if (!anexar(n, esperar(p, CAT_NUM_INT, ESP_NUM))) return falha(n);
    return n;
}

/* <endereco> ::= ID | NUM_INT SMB_OPA REG SMB_CPA */
static No *endereco(Parser *p){
    No *n = noInterno(p, "<endereco>");
    if (verifica(p, CAT_ID)) {
        anexar(n, consumirAtual(p));
        return n;
    }
    if (!verifica(p, CAT_NUM_INT)) {
        erroSintatico(p, "ID ou NUM_INT");
        return falha(n);
    }
    anexar(n, consumirAtual(p));
    if (!anexar(n, esperar(p, CAT_ABRE, ESP_ABRE))) return falha(n);
    if (!anexar(n, esperar(p, CAT_REG, ESP_REG))) return falha(n);
    if (!anexar(n, esperar(p, CAT_FECHA, ESP_FECHA))) return falha(n);
    return n;
}

/* <op_memoria> ::= REG SMB_COM <endereco> */
static No *op_memoria(Parser *p){
    No *n = noInterno(p, "<op_memoria>");
    if (!anexar(n, esperar(p, CAT_REG, ESP_REG))) return falha(n);
    if (!anexar(n, esperar(p, CAT_VIRGULA, ESP_VIRG))) return falha(n);
    if (!anexar(n, endereco(p))) return falha(n);
    return n;
}

/* <op_desvio> ::= REG SMB_COM REG SMB_COM ID */
static No *op_desvio(Parser *p){
    No *n = noInterno(p, "<op_desvio>");
    if (!anexar(n, esperar(p, CAT_REG, ESP_REG))) return falha(n);
    if (!virgulaEReg(p, n)) return falha(n);
    if (!anexar(n, esperar(p, CAT_VIRGULA, ESP_VIRG))) return falha(n);
    if (!anexar(n, esperar(p, CAT_ID, ESP_ID))) return falha(n);
    return n;
}

/* <op_salto> ::= ID */
static No *op_salto(Parser *p){
    No *n = noInterno(p, "<op_salto>");
    if (!anexar(n, esperar(p, CAT_ID, ESP_ID))) return falha(n);
    return n;
}

/* ---------- grupos de instrucao ----------
   Cada funcao consome o mnemonico (token atual, ja verificado por instrucao())
   e depois os operandos. Nenhuma consome o EOL: isso e da linha_codigo(). */

/* Consome o mnemonico e devolve o no do grupo (nome) ja com o mnemonico como filho. */
static No *inicioGrupo(Parser *p, const char *nome){
    No *n = noInterno(p, nome);
    anexar(n, consumirAtual(p));
    return n;
}

/* <instrucao_tres_registradores> ::= (ADD|ADDU|SUB|SUBU|MUL|AND|OR|XOR|NOR|SLT) <op_tres_registradores> */
static No *instrucao_tres_registradores(Parser *p){
    No *n = inicioGrupo(p, "<instrucao_tres_registradores>");
    if (!anexar(n, op_tres_registradores(p))) return falha(n);
    return n;
}

/* <instrucao_dois_registradores> ::= (DIV|MOVE) <op_dois_registradores> */
static No *instrucao_dois_registradores(Parser *p){
    No *n = inicioGrupo(p, "<instrucao_dois_registradores>");
    if (!anexar(n, op_dois_registradores(p))) return falha(n);
    return n;
}

/* <instrucao_um_registrador> ::= (MFHI|MFLO|JR) <op_um_registrador> */
static No *instrucao_um_registrador(Parser *p){
    No *n = inicioGrupo(p, "<instrucao_um_registrador>");
    if (!anexar(n, op_um_registrador(p))) return falha(n);
    return n;
}

/* <instrucao_imediato> ::= INS_ADDI REG SMB_COM REG SMB_COM NUM_INT */
static No *instrucao_imediato(Parser *p){
    No *n = inicioGrupo(p, "<instrucao_imediato>");
    if (!anexar(n, esperar(p, CAT_REG, ESP_REG))) return falha(n);
    if (!virgulaEReg(p, n)) return falha(n);
    if (!anexar(n, esperar(p, CAT_VIRGULA, ESP_VIRG))) return falha(n);
    if (!anexar(n, esperar(p, CAT_NUM_INT, ESP_NUM))) return falha(n);
    return n;
}

/* <instrucao_deslocamento> ::= (SLL|SRL) <op_deslocamento> */
static No *instrucao_deslocamento(Parser *p){
    No *n = inicioGrupo(p, "<instrucao_deslocamento>");
    if (!anexar(n, op_deslocamento(p))) return falha(n);
    return n;
}

/* <instrucao_li> ::= INS_LI REG SMB_COM NUM_INT */
static No *instrucao_li(Parser *p){
    No *n = inicioGrupo(p, "<instrucao_li>");
    if (!anexar(n, esperar(p, CAT_REG, ESP_REG))) return falha(n);
    if (!anexar(n, esperar(p, CAT_VIRGULA, ESP_VIRG))) return falha(n);
    if (!anexar(n, esperar(p, CAT_NUM_INT, ESP_NUM))) return falha(n);
    return n;
}

/* <instrucao_la> ::= INS_LA REG SMB_COM ID */
static No *instrucao_la(Parser *p){
    No *n = inicioGrupo(p, "<instrucao_la>");
    if (!anexar(n, esperar(p, CAT_REG, ESP_REG))) return falha(n);
    if (!anexar(n, esperar(p, CAT_VIRGULA, ESP_VIRG))) return falha(n);
    if (!anexar(n, esperar(p, CAT_ID, ESP_ID))) return falha(n);
    return n;
}

/* <instrucao_memoria> ::= (LW|SW|LB|SB|LH|SH) <op_memoria> */
static No *instrucao_memoria(Parser *p){
    No *n = inicioGrupo(p, "<instrucao_memoria>");
    if (!anexar(n, op_memoria(p))) return falha(n);
    return n;
}

/* <instrucao_desvio> ::= (BEQ|BNE|BLT|BGT|BLE|BGE) <op_desvio> */
static No *instrucao_desvio(Parser *p){
    No *n = inicioGrupo(p, "<instrucao_desvio>");
    if (!anexar(n, op_desvio(p))) return falha(n);
    return n;
}

/* <instrucao_salto> ::= (J|JAL) <op_salto> */
static No *instrucao_salto(Parser *p){
    No *n = inicioGrupo(p, "<instrucao_salto>");
    if (!anexar(n, op_salto(p))) return falha(n);
    return n;
}

/* <instrucao_sem_operandos> ::= INS_SYSCALL | INS_NOP */
static No *instrucao_sem_operandos(Parser *p){
    return inicioGrupo(p, "<instrucao_sem_operandos>");
}

/* <instrucao> ::= <instrucao_...>   (grupo escolhido pelo mnemonico atual) */
No *instrucao(Parser *p){
    No *n = noInterno(p, "<instrucao>");
    No *f = NULL;

    switch (grupoDoMnemonico(atual(p)->categoria)) {
        case GRP_TRES_REG:      f = instrucao_tres_registradores(p);  break;
        case GRP_DOIS_REG:      f = instrucao_dois_registradores(p);  break;
        case GRP_UM_REG:        f = instrucao_um_registrador(p);      break;
        case GRP_IMEDIATO:      f = instrucao_imediato(p);            break;
        case GRP_DESLOCAMENTO:  f = instrucao_deslocamento(p);        break;
        case GRP_LI:            f = instrucao_li(p);                  break;
        case GRP_LA:            f = instrucao_la(p);                  break;
        case GRP_MEMORIA:       f = instrucao_memoria(p);             break;
        case GRP_DESVIO:        f = instrucao_desvio(p);              break;
        case GRP_SALTO:         f = instrucao_salto(p);               break;
        case GRP_SEM_OPERANDOS: f = instrucao_sem_operandos(p);       break;
        default:
            erroSintatico(p, "instrucao");
            return falha(n);
    }

    if (!anexar(n, f)) return falha(n);
    return n;
}
/* ===== fim do bloco da Pessoa 3 ===== */
