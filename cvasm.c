#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char ahc = 0;
char *tok[40];
int ntok;
char tokbuf[300];
unsigned line;
char *gfilename;
char mem[64*1024];
unsigned org = 0, nmem = 0;
int ti;
char idbuf[8192];
char *idbufp = idbuf;
char *vals[1000];
int   vala[1000];
int nvals = 0;

const char *delim = "\n+-*/%&|^():;,.\"'=";

const char *ins4[] = {
    "bl", "movw", "movwu", "relw", 0,
};

const char *ins6[] = {
    "movs", "movsu", "rels", 0,
};

const char *ins0[] = {
    "nop", "swi", 0,
};

const char *altregs[] = {
    "ir", "zr", "cr", "bp", "sp", "lr", "pc",
};

const char *insrr[] = {
    "ll", "lw", "lb", "lwu", "lbu", "lnl", "lnw", "lnb", "lnwu", "lnbu",
    "gt", "lt", "gtu", "ltu", "eq", "",
    "mov", "add", "sub", "and", "or", "xor", "asr", "lsr", "lsl",
    "mul", "mulu", "div", "divu", "rem", "remu", "",
    "sl", "sw", "sb", "dsl", "dsw", "dsb", "snl", "snw", "snb", 0,
};

const char *insr[] = {
    "br", "inv", "neg", "not", 0,
};

void parseBuf(FILE *fp, char *buf) {
    int c, i = 0;
    if(ahc) { c = ahc; ahc = 0; } else c = fgetc(fp);
    for(;;) {
        if(strchr(delim, c)) {
            if(i) { buf[i] = 0; ahc = c; } else { buf[0] = c; buf[1] = 0; }
            return;
        } else if(c <= 32) {
            if(i || c == EOF) { buf[i] = 0; return; }
        } else buf[i++] = c;
        c = fgetc(fp);
    }
}

void perr() {
    if(gfilename) printf("%s:%d: ", gfilename, line);
    printf("error: ");
}

void err(const char *s) {
    perr(); printf("%s\n", s); exit(1);
}

void parseQuote(FILE *fp, char *buf, char q) {
    int c;
    for(;;) {
        c = fgetc(fp);
        if(c == q) break;
        if(c == '\\') c = fgetc(fp);
        if(c == EOF || c == '\n') err("unterminated quote");
        *(buf++) = c;
    }
    *buf = 0;
}

void parseLine(FILE *fp) {
    char *b = tokbuf;
    for(ntok = 0; ntok < 10; ntok++) tok[ntok] = 0;
    ntok = 0;
    for(;;) {
        parseBuf(fp, b);
        if(!strcmp(b, "\n")) {
            if(ntok) { ahc = '\n'; break; } else line++;
        } else if(!strcmp(b, ":")) { if(ntok) break; }
        else if(!strcmp(b, ";")) {
            do {
                parseBuf(fp, b);
                if(!b[0]) { tok[ntok] = ""; return; }
            } while(strcmp(b, "\n"));
            if(ntok) { ahc = '\n'; break; } else line++;
        } else if(!b[0]) break;
        else {
            if(!strcmp(b, "\"")) parseQuote(fp, b+1, '"');
            else if(!strcmp(b, "'")) {
                *b = '"'; parseQuote(fp, b+1, '\'');
            }
            tok[ntok++] = b;
            b += strlen(b)+1;
        }
    }
    tok[ntok] = "";
}

FILE *openFile(char *filename, const char *m) {
    FILE *fp = fopen(filename, m);
    if(!fp) { perr(); printf("failed to open %s\n", filename); exit(1); }
    gfilename = filename;
    line = 1;
    return fp;
}

void expect(int i, char *s) {
    if(strcmp(tok[i], s)) { perr(); printf("expected %s\n", s); exit(1); }
}

int bin(char *s, int *n) {
    do {
        *n *= 2;
        if(*s == '1') (*n)++;
        else if(*s != '0') return 0;
    } while(*(++s));
    return 1;
}

int oct(char *s, int *n) {
    do {
        *n *= 8;
        if(*s >= '0' && *s <= '7') *n += *s - '0';
        else return 0;
    } while(*(++s));
    return 1;
}

int hex(char *s, int *n) {
    do {
        *n *= 16;
        if(*s >= '0' && *s <= '9') *n += *s - '0';
        else if(*s >= 'a' && *s <= 'f') *n += *s - 'a' + 10;
        else if(*s >= 'A' && *s <= 'F') *n += *s - 'A' + 10;
        else return 0;
    } while(*(++s));
    return 1;
}

int number(char *s, int *n) {
    *n = 0;
    if(*s == '0') {
        if(s[1] == 'x') return hex(s+2, n);
        if(s[1] == 'b') return bin(s+2, n);
        return oct(s, n);
    }
    do {
        *n *= 10;
        if(*s >= '0' && *s <= '9') *n += *s - '0';
        else return 0;
    } while(*(++s));
    return 1;
}

int findVal(char *s, int *v) {
    int i;
    for(i = 0; i < nvals; i++)
        if(!strcmp(vals[i], s)) { *v = vala[i]; return 1; }
    return 0;
}

int evalOr();

int evalUnary() {
    int n;
    if(!strcmp(tok[ti], "-")) { ti++; return -evalUnary(); }
    if(!strcmp(tok[ti], "~")) { ti++; return ~evalUnary(); }
    if(!strcmp(tok[ti], "(")) {
        ti++; n = evalOr(); expect(ti++, ")"); return n;
    }
    if(*tok[ti] == '"' && tok[ti][1] && !tok[ti][2]) return tok[ti++][1];
    if(!strcmp(tok[ti], ".")) { ti++; return org; }
    if(number(tok[ti], &n)) { ti++; return n; }
    if(findVal(tok[ti], &n)) { ti++; return n; }
    err("expected value");
}
int evalDiv() {
    int n = evalUnary();
    if(!strcmp(tok[ti], "*")) { ti++; n *= evalDiv(); }
    else if(!strcmp(tok[ti], "/")) { ti++; n /= evalDiv(); }
    else if(!strcmp(tok[ti], "%")) { ti++; n %= evalDiv(); }
    return n;
}
int evalAdd() {
    int n = evalDiv();
    if(!strcmp(tok[ti], "+")) { ti++; n += evalAdd(); }
    else if(!strcmp(tok[ti], "-")) { ti++; n -= evalAdd(); }
    return n;
}
int evalAnd() {
    int n = evalAdd();
    if(!strcmp(tok[ti], "&")) { ti++; n &= evalAnd(); }
    return n;
}
int evalXor() {
    int n = evalAnd();
    if(!strcmp(tok[ti], "^")) { ti++; n ^= evalXor(); }
    return n;
}
int evalOr() {
    int n = evalXor();
    if(!strcmp(tok[ti], "|")) { ti++; n |= evalOr(); }
    return n;
}

int eval(int i) {
    ti = i;
    return evalOr();
}

void nli(int i) {
    if(i < ntok) err("expected end of line");
}

void nl() {
    nli(ti);
}

int countD() {
    int i, t = (ntok != 0);
    for(i = 1; i < ntok; i++) {
        if(*tok[i] == '"') t += strlen(tok[i])-2;
        else if(!strcmp(tok[i], ",")) t++;
    }
    return t;
}

int strindex(const char **strs, char *s) {
    int i;
    for(i = 0; strs[i]; i++) if(!strcmp(strs[i], s)) return i;
    return -1;
}

void alignOrg(int al) {
    org += (al-org%al)%al;
}

void addVal(char *s, int v) {
    int n;
    if(strchr(delim, *s) || number(s, &n)) err("invalid identifier");
    if(findVal(s, &n)) err("name already taken");
    vals[nvals] = idbufp;
    vala[nvals++] = v;
    strcpy(idbufp, s);
    idbufp += strlen(idbufp)+1;
}

char *string(char *s) {
    if(*s != '"') err("expected string");
    return s+1;
}

void pass1(char *filename) {
    FILE *fp = openFile(filename, "r");
    int n;
    for(;;) {
        parseLine(fp);
        if(!ntok) break;
        /*printf("%d", line);
        for(n = 0; n < ntok; n++) printf(" %s", tok[n]);
        printf("\n");*/
        if(ntok == 1) {
            if(strindex(ins0, tok[0]) == -1) addVal(tok[0], org);
            else org += 2;
        } else if(!strcmp(tok[1], "=")) { addVal(tok[0], eval(2)); nl(); }
        else if(!strcmp(tok[0], "include")) {
            n = line; pass1(string(tok[1])); nli(2);
            line = n; gfilename = filename;
        } else if(!strcmp(tok[0], "align")) { alignOrg(eval(1)); nl(); }
        else if(!strcmp(tok[0], "org")) { org = eval(1); nl(); }
        else if(!strcmp(tok[0], "db")) org += countD();
        else if(!strcmp(tok[0], "dw")) org += countD()*2;
        else if(!strcmp(tok[0], "dl")) org += countD()*4;
        else if(strindex(ins4, tok[0]) != -1) org += 4;
        else if(strindex(ins6, tok[0]) != -1) org += 6;
        else org += 2;
    }
    fclose(fp); gfilename = 0;
}

void db(int b) {
    mem[nmem++] = b; org++;
}

void dw(int w) {
    *(short*)&mem[nmem] = w; nmem += 2; org += 2;
}

void dl(int l) {
    *(int*)&mem[nmem] = l; nmem += 2; org += 2;
}

void align(int al) {
    int i;
    al = (al-org%al)%al;
    for(i = 0; i < al; i++) db(0);
}

void declare(void (*d)(int)) {
    int i;
    ti = 1;
    for(;;) {
        if(tok[ti][0] == '"' && strlen(tok[ti]) != 2) {
            for(i = 1; tok[ti][i]; i++) d(tok[ti][i]);
            ti++;
        } else d(evalOr());
        if(ti == ntok) break;
        expect(ti++, ",");
    }
}

int regi(int i) {
    char *s = tok[i];
    if((i = strindex(altregs, s)) != -1) return i+9;
    if(s[0] != 'r') return -1;
    if(s[1] == '1' && s[2] >= '0' && s[2] <= '5' && !s[3])
        return 10+s[2]-'0';
    if(s[1] < '0' || s[1] > '9' || s[2]) return -1;
    return s[1]-'0';
}

int reg(int i) {
    int r = regi(i);
    if(r == -1) err("expected register");
    return r;
}

int range(int n, int lb, int ub) {
    if(n >= lb && n <= ub) return n;
    err("value out of range");
}

int wa(int n) {
    if(n&1) err("value not word-aligned");
    return n/2;
}

void comma() {
    expect(2, ",");
}

void pass2(char *filename) {
    FILE *fp = openFile(filename, "r");
    int n, r;
    for(;;) {
        parseLine(fp);
        if(!ntok) break;
        if(ntok == 1 && strindex(ins0, tok[0]) == -1);
        else if(!strcmp(tok[1], "="));
        else if(!strcmp(tok[0], "align")) align(eval(1));
        else if(!strcmp(tok[0], "org")) org = eval(1);
        else if(!strcmp(tok[0], "include")) {
            n = line; pass2(tok[1]+1);
            line = n; gfilename = filename;
        } else if(!strcmp(tok[0], "db")) declare(db);
        else if(!strcmp(tok[0], "dw")) declare(dw);
        else if(!strcmp(tok[0], "dl")) declare(dl);
        else if((n = strindex(insrr, tok[0])) != -1) {
            nli(4); comma();
            dw((0xd000+(n<<8))|reg(1)<<4|reg(3));
        } else if((n = strindex(insr, tok[0])) != -1) {
            nli(2); dw(0xff00|n<<4|reg(1));
        } else if(!strcmp(tok[0], "b")) {
            dw(0xa000|
                wa(range(eval(1),org+2-4096,org+2+4094)-org-2)&0xfff);
            nl();
        } else if(!strcmp(tok[0], "bz")) {
            comma();
            dw(0xb000|reg(1)<<8|
                wa(range(eval(3),org+2-256,org+2+254)-org-2)&0xff);
            nl();
        } else if(!strcmp(tok[0], "bnz")) {
            comma();
            dw(0xc000|reg(1)<<8|
                wa(range(eval(3),org+2-256,org+2+254)-org-2)&0xff);
            nl();
        } else if(!strcmp(tok[0], "movb")) {
            comma();
            dw(0x1000|reg(1)<<8|range(eval(3), -128, 127)&0xff); nl();
        } else if(!strcmp(tok[0], "movbu")) {
            comma();
            dw(0x2000|reg(1)<<8|range(eval(3), 0, 255)); nl();
        } else if(!strcmp(tok[0], "movw")) {
            r = reg(1); comma(); n = range(eval(3), -32768, 32767);
            nl();
            dw(0x1000|r<<8|(n>>8)&0xff); dw(0x3000|r<<8|n&0xff);
        } else if(!strcmp(tok[0], "movwu")) {
            r = reg(1); comma(); n = range(eval(3), 0, 65535); nl();
            dw(0x2000|r<<8|n>>8); dw(0x3000|r<<8|n&0xff);
        } else if(!strcmp(tok[0], "movs")) {
            r = reg(1); comma();
            n = range(eval(3), -8388608, 8388607); nl();
            dw(0x1000|r<<8|(n>>16)&0xff); dw(0x3000|r<<8|(n>>8)&0xff);
            dw(0x3000|r<<8|n&0xff);
        } else if(!strcmp(tok[0], "movsu")) {
            r = reg(1); comma();
            n = range(eval(3), 0, 16777215); nl();
            dw(0x2000|r<<8|n>>16); dw(0x3000|r<<8|(n>>8)&0xff);
            dw(0x3000|r<<8|n&0xff);
        } else if(!strcmp(tok[0], "rel")) {
            comma();
            dw(0x4000|reg(1)|range(eval(3), 0, 255)); nl();
        } else if(!strcmp(tok[0], "relw")) {
            comma(); n = range(eval(3), 0, 65535); nl();
            dw(0x2000|14<<8|n>>8); dw(0x5000|reg(1)<<8|n&0xff);
        } else if(!strcmp(tok[0], "rels")) {
            comma(); n = range(eval(3), 0, 16777215); nl();
            dw(0x2000|14<<8|n>>16); dw(0x3000|14<<8|(n>>8)&0xff);
            dw(0x5000|reg(1)<<8|n&0xff);
        } else if(!strcmp(tok[0], "bl")) {
            n = wa(range(eval(1), 0, 2097150)); nl();
            dw(0x2000|14<<8|n>>12); dw(0x9000|n&0xfff);
        } else if(!strcmp(tok[0], "addi")) {
            comma();
            dw(0x0000|reg(1)<<8|range(eval(3), -128, 127)&0xff); nl();
        } else if(!strcmp(tok[0], "andi")) {
            comma(); dw(0x7000|reg(1)<<8|range(eval(3), 0, 255)); nl();
        } else if(!strcmp(tok[0], "ori")) {
            comma(); dw(0x8000|reg(1)<<8|range(eval(3), 0, 255)); nl();
        } else if(!strcmp(tok[0], "adr")) {
            comma();
            dw(0x6000|reg(1)<<8|wa(range(eval(3),org+2,org+512)-org-2)&0xff);
            nl();
        } else if(!strcmp(tok[0], "asri")) {
            comma(); dw(0xf900|reg(1)<<4|range(eval(3), 1, 16)-1); nl();
        } else if(!strcmp(tok[0], "lsri")) {
            comma(); dw(0xfa00|reg(1)<<4|range(eval(3), 1, 16)-1); nl();
        } else if(!strcmp(tok[0], "asri")) {
            comma(); dw(0xfb00|reg(1)<<4|range(eval(3), 1, 16)-1); nl();
        } else if(!strcmp(tok[0], "muli")) {
            comma(); dw(0xfc00|reg(1)<<4|range(eval(3), 3, 18)-3); nl();
        } else if(!strcmp(tok[0], "divi")) {
            comma(); dw(0xfd00|reg(1)<<4|range(eval(3), 3, 18)-3); nl();
        } else if(!strcmp(tok[0], "divui")) {
            comma(); dw(0xfe00|reg(1)<<4|range(eval(3), 3, 18)-3); nl();
        } else if(!strcmp(tok[0], "port")) {
            dw(0xff40|range(eval(1), 0, 15)); nl();
        } else if(!strcmp(tok[0], "swi")) {
            dw(0xff40); nli(1);
        } else if(!strcmp(tok[0], "nop")) {
            dw(0x0000); nli(1);
        } else err("expected instruction");
    }
    fclose(fp); gfilename = 0;
}

void saveFile(char *filename) {
    int i;
    FILE *fp = openFile(filename, "wb");
    fwrite(mem, 1, nmem, fp);
    fclose(fp);
    for(i = 0; i < nvals; i++) printf("0x%.8x %s\n", vala[i], vals[i]);
    printf("assembled %d bytes\n", nmem);
}

int main(int argc, char **args) {
    int i;
    if(argc < 3) {
        printf("usage: cvasm <file1.s,file2.s,...> <fileout>\n"); return 1;
    }
    for(i = 1; i < argc-1; i++) pass1(args[i]);
    org = 0;
    for(i = 1; i < argc-1; i++) pass2(args[i]);
    saveFile(args[argc-1]);
    return 0;
}

