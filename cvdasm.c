#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *ins1[] = {
    "addi", "movb", "movbu", "movlo", "rel", "rello", "adr",
    "andi", "ori", "bllo", "b", "bnz",
};

const char *ins2[] = {
    "ll", "lw", "lb", "lwu", "lbu", "lnl", "lnw", "lnb", "lnwu", "lnbu",
    "gt", "lt", "gtu", "ltu", "eq",
};

const char *ins3[] = {
    "mov", "add", "sub", "and", "or", "xor", "asr", "lsr", "lsl",
    "mul", "mulu", "div", "divu", "rem", "remu",
};

const char *ins4[] = {
    "sl", "sw", "sb", "dsl", "dsw", "dsb", "snl", "snw", "snb",
    "asri", "lsri", "lsli", "divi", "divui",
};

const char *ins5[] = {
    "br", "inv", "neg", "not", "port",
};

void putb(char b) {
    if(b > 32 && b < 127) fputc(b, stdout);
    else fputc('.', stdout);
}

void dasm(char *filename) {
    unsigned org = 64;
    unsigned short ins, u12;
    char n3, n2, n1, n0;
    short s12;
    char b;
    unsigned char ub;
    unsigned short p = 0;
    FILE *fp;

    fp = fopen(filename, "rb");
    if(!fp) { printf("failed to open %s\n", filename); return; }

    while(fread(&ins, 2, 1, fp)) {
        printf("%.8X %.2X %.2X ", org, ins&0xff, ins>>8);
        putb(ins&0xff); putb(ins>>8); printf(" ");
        org += 2;

        u12 = ins&0xfff; s12 = u12<<4; s12 >>= 4;
        n0=ins&0xf; n1=(ins>>4)&0xf; n2=(ins>>8)&0xf; n3=(ins>>12)&0xf;
        ub = b = ins;

        switch(n3) {
        default: printf("%s r%d,0x%.2x", ins1[n3], n2, ub); break;
        case 11: printf("bz r%d,%.8X", n2, org+b*2); break;
        case 12: printf("bnz r%d,%.8X", n2, org+b*2); break;
        case 10: printf("b %.8X", org+s12*2); break;
        case 9:
            printf("bllo 0x%.3x", u12);
            if(p>>12 == 2) printf(" (bl %.8X)", (p&0xff)<<13|u12<<1);
            break;
        case 6: printf("adr r%d,%.8X", n2, org+b*2); break;

        case 13:
            if(n2 == 15) printf("???");
            else printf("%s r%d,r%d", ins2[n2], n1, n0);
            break;

        case 14:
            if(n2 == 15) printf("???");
            else printf("%s r%d,r%d", ins3[n2], n1, n0);
            break;

        case 15:
            if(n2 <= 8) printf("%s r%d,r%d", ins4[n2], n1, n0);
            else if(n2 <= 11) printf("%s r%d,%d", ins4[n2], n1, n0+1);
            else if(n2 < 15) printf("%s r%d,%d", ins4[n2], n1, n0+3);
            else if(n1 <= 3) printf("%s r%d", ins5[n1], n0);
            else if(n1 == 4) printf("port %d", n0);
            else printf("???");
            break;
        }

        p = ins;
        printf("\n");
    }

    fclose(fp);
}

int main(int argc, char **args) {
    int i;
    for(i = 1; i < argc; i++) dasm(args[i]);
    return 0;
}
