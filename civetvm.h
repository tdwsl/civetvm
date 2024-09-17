#ifndef CIVETVM_H
#define CIVETVM_H

/* memory size */
#define CIVET_MEMSZ 0x200000

#define CIVET_MAXPORTS 32

extern char cv_mem[];
extern int *cv_reg;

int cv_ll(unsigned a);
short cv_lw(unsigned a);
char cv_lb(unsigned a);

void cv_sl(unsigned a, int l);
void cv_sw(unsigned a, short w);
void cv_sb(unsigned a, char b);

#define cv_lwu(a) (unsigned short)cv_lw(a)
#define cv_lbu(a) (unsigned char)cv_lb(a)

/* add i/o functionality, called by port instruction */
/* each port can have 16 operations max */
/* return 1 on success */
int cv_connect(void (*port)(int), int id);

void cv_run();

#endif

