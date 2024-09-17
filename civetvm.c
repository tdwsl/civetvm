#include "civetvm.h"
#include <stdio.h>

char cv_mem[CIVET_MEMSZ];
int *cv_reg = (int*)cv_mem;
static int *reg;
static unsigned offs;
static unsigned memsz;
static char quit;

static void port0(int op);

static void (*ports[CIVET_MAXPORTS])(int) = {port0,0};
static int portids[CIVET_MAXPORTS] = {0x30564943,0}; /* CVT0 */
static int nports = 1;

int cv_ll(unsigned a) {
    if(cv_reg[11] && a > memsz-4) { cv_reg[9] = -1; cv_reg[11] = 0; }
    else if(offs+a > CIVET_MEMSZ-4) return 0;
    else return *(int*)&cv_mem[offs+a];
}
short cv_lw(unsigned a) {
    if(cv_reg[11] && a > memsz-2) { cv_reg[9] = -1; cv_reg[11] = 0; }
    else if(offs+a > CIVET_MEMSZ-2) return 0;
    else return *(short*)&cv_mem[offs+a];
}
char cv_lb(unsigned a) {
    if(cv_reg[11] && a > memsz-1) { cv_reg[9] = -1; cv_reg[11] = 0; }
    else if(offs+a > CIVET_MEMSZ-1) return 0;
    else return (char)cv_mem[offs+a];
}
void cv_sl(unsigned a, int l) {
    if(cv_reg[11] && a > memsz-4) { cv_reg[9] = -1; cv_reg[11] = 0; }
    else if(offs+a <= CIVET_MEMSZ-4) *(int*)&cv_mem[offs+a] = l;
}
void cv_sw(unsigned a, short w) {
    if(cv_reg[11] && a > memsz-2) { cv_reg[9] = -1; cv_reg[11] = 0; }
    else if(offs+a <= CIVET_MEMSZ-2) *(short*)&cv_mem[offs+a] = w;
}
void cv_sb(unsigned a, char b) {
    if(cv_reg[11] && a > memsz-1) { cv_reg[9] = -1; cv_reg[11] = 0; }
    else if(offs+a <= CIVET_MEMSZ-1) cv_mem[offs+a] = b;
}

int cv_connect(void (*port)(int), int id) {
    int i;
    if(nports >= CIVET_MAXPORTS) return 0;
    portids[nports] = id;
    ports[nports++] = port;
    return 1;
}

static void port0(int op) {
    switch(op) {
    case 0: quit = 1; break;
    case 1: if((unsigned)cv_reg[1] >= nports) cv_reg[1] = 0;
        else cv_reg[1] = portids[(unsigned)cv_reg[1]]; break;
    case 2: cv_reg[1] = nports; break;
    case 3: cv_reg[1] = CIVET_MEMSZ; break;
    }
}

void cv_run() {
    unsigned short ins;
    char b;
    unsigned char bu;
    unsigned char n0, n1, n2, n3;
    unsigned short u12;
    short s12;

    quit = 0;
    while(!quit) {
        if(cv_reg[11]) {
            offs = cv_reg[12];
            memsz = cv_reg[10];
            reg = (int*)(cv_mem+offs);
            cv_reg[11]--;
        } else { offs = 0; reg = cv_reg; }

        //printf("%.8x %.4x\n", reg[15], cv_lwu(reg[15]));

        ins = cv_lw(reg[15]); reg[15] += 2;
        n0 = ins&0xf; n1 = (ins>>4)&0xf;n2 = (ins>>8)&0xf;
        n3 = (ins>>12)&0xf;
        u12 = ins&0x0fff; s12 = ins<<4; s12 >>= 4;
        b = bu = ins;

        switch(n3) { /* instructions 1 */
        case 0: /* add r,# */ reg[n2] += b; break;
        case 1: /* movb r,# */ reg[n2] = b; break;
        case 2: /* movbu r,#u */ reg[n2] = bu; break;
        case 3: /* movlo r,#u */ reg[n2] = reg[n2]<<8|bu; break;
        case 4: /* rel r,#u */ reg[n2] = reg[12]+bu; break;
        case 5: /* rello r,#u */
            reg[n2] = reg[12]+(reg[14]<<8|bu); break;
        case 6: /* adr r,#u*2 */ reg[n2] = reg[15]+bu*2; break;
        case 7: /* and r,#u */ reg[n2] &= bu; break;
        case 8: /* or r,#u */ reg[n2] |= bu; break;
        case 9: /* bllo #12u */ ins = reg[14]; reg[14] = reg[15];
            reg[15] = ins<<13|u12<<1; break;
        case 10: /* b #12 */ reg[15] += s12*2; break;
        case 11: /* bz r,# */ if(!reg[n2]) reg[15] += b*2; break;
        case 12: /* bnz r,# */ if(reg[n2]) reg[15] += b*2; break;
        case 13:
            switch(n2) { /* instructions 2 */
            case 0: /* ll r,r */ reg[n1] = cv_ll(reg[n0]); break;
            case 1: /* lw r,r */ reg[n1] = cv_lw(reg[n0]); break;
            case 2: /* lb r,r */ reg[n1] = cv_lb(reg[n0]); break;
            case 3: /* lwu r,r */ reg[n1] = cv_lwu(reg[n0]); break;
            case 4: /* lbu r,r */ reg[n1] = cv_lbu(reg[n0]); break;
            case 5: /* lnl r,r */ reg[n1] = cv_ll((reg[n0]+=4)-4); break;
            case 6: /* lnw r,r */ reg[n1] = cv_lw((reg[n0]+=2)-2); break;
            case 7: /* lnb r,r */ reg[n1] = cv_lb(reg[n0]++); break;
            case 8: /* lnwu r,r */ reg[n1] = cv_lwu((reg[n0]+=2)-2);
            case 9: /* lnbu r,r */ reg[n1] = cv_lbu(reg[n0]++); break;
            case 10: /* gt r,r */ reg[14] = reg[n1] > reg[n0]; break;
            case 11: /* lt r,r */ reg[14] = reg[n1] < reg[n0]; break;
            case 12: /* gtu r,r */
                reg[14] = (unsigned)reg[n1]>(unsigned)reg[n0]; break;
            case 13: /* ltu r,r */
                reg[14] = (unsigned)reg[n1]<(unsigned)reg[n0]; break;
            case 14: /* eq r,r */ reg[14] = reg[n1]==reg[n0]; break;
            }
            break;
        case 14:
            switch(n2) { /* instructions 3 */
            case 0: /* mov r,r */ reg[n1] = reg[n0]; break;
            case 1: /* add r,r */ reg[n1] += reg[n0]; break;
            case 2: /* sub r,r */ reg[n1] -= reg[n0]; break;
            case 3: /* and r,r */ reg[n1] &= reg[n0]; break;
            case 4: /* or  r,r */ reg[n1] |= reg[n0]; break;
            case 5: /* xor r,r */ reg[n1] ^= reg[n0]; break;
            case 6: /* asr r,r */ reg[n1] >>= reg[n0]; break;
            case 7: /* lsr r,r */
                *(unsigned*)&reg[n1] >>= (unsigned)reg[n0]; break;
            case 8: /* lsl r,r */ reg[n1] <<= reg[n0]; break;
            case 9: /* mul r,r */ reg[n1] *= reg[n0]; break;
            case 10: /* mulu r,r */
                *(unsigned*)&reg[n1] *= (unsigned)reg[n0]; break;
            case 11: /* div  r,r */ reg[n1] /= reg[n0]; break;
            case 12: /* divu r,r */
                *(unsigned*)&reg[n1] /= (unsigned)reg[n0]; break;
            case 13: /* rem  r,r */ reg[n1] %= reg[n0]; break;
            case 14: /* remu r,r */
                *(unsigned*)&reg[n1] %= (unsigned)reg[n0]; break;
            }
            break;
        case 15:
            switch(n2) { /* instructions 4 */
            case 0: /* sl r,r */ cv_sl(reg[n0], reg[n1]); break;
            case 1: /* sw r,r */ cv_sw(reg[n0], reg[n1]); break;
            case 2: /* sb r,r */ cv_sb(reg[n0], reg[n1]); break;
            case 3: /* dsl r,r */ cv_sl(reg[n0]-=4, reg[n1]); break;
            case 4: /* dsw r,r */ cv_sw(reg[n0]-=2, reg[n1]); break;
            case 5: /* dsb r,r */ cv_sb(--reg[n0], reg[n1]); break;
            case 6: /* snl r,r */ cv_sl((reg[n0]+=4)-4, reg[n1]); break;
            case 7: /* snw r,r */ cv_sw((reg[n0]+=2)-2, reg[n1]); break;
            case 8: /* snb r,r */ cv_sb(reg[n0]++, reg[n1]); break;
            case 9: /* asri r,#+1 */ reg[n1] >>= n0+1; break;
            case 10: /* lsri r,#+1 */ *(unsigned*)&reg[n1] >>= n0+1; break;
            case 11: /* lsli r,#+1 */ reg[n1] <<= n0+1; break;
            case 12: /* muli r,#+3 */ reg[n1] *= n0+3; break;
            case 13: /* divi r,#+3 */ reg[n1] /= n0+3; break;
            case 14: /* divui r,#+3 */ *(unsigned*)&reg[n1] /= n0+3; break;
            case 15:
                switch(n1) { /* instructions 5 */
                case 0: /* br r */ reg[14] = reg[15];
                    reg[15] = reg[n0]; break;
                case 1: /* inv r */ reg[n0] = ~reg[n0]; break;
                case 2: /* neg r */ reg[n0] = -reg[n0]; break;
                case 3: /* not r */ reg[n0] = !reg[n0]; break;
                case 4: /* port # */
                    if(offs) {
                        if(cv_reg[11]) {
                            cv_reg[9] = cv_reg[11]; cv_reg[11] = 0;
                        } else reg[15] -= 2;
                    } else if((unsigned)cv_reg[0] >= CIVET_MAXPORTS);
                    else if(ports[cv_reg[0]]) ports[cv_reg[0]](n0);
                    break;
                }
                break;
            }
            break;
        }
    }
}

