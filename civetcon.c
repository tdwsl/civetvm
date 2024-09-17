#include "civetvm.h"
#include "civetcon.h"
#include <stdio.h>

static void civetcon(int op) {
    switch(op) {
    case 0: cv_reg[1] = fgetc(stdin); break;
    case 1: fputc(cv_reg[1], stdout); break;
    }
}

void connect_civetcon() {
    cv_connect(civetcon, 0x6e6f6363);
}

