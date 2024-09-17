#include "civetvm.h"
#include "civetcon.h"
#include "civetdisk.h"
#include <stdio.h>

const char *kname = "boot.k";

int main(int argc, char **args) {
    FILE *fp;
    int i;
    connect_civetcon();
    for(i = 1; i < argc; i++)
        civetdisk_load(connect_civetdisk(), args[i]);
    fp = fopen(kname, "rb");
    if(!fp) { printf("failed to open %s\n", kname); return 1; }
    fread(cv_mem+64, 1, CIVET_MEMSZ-16, fp);
    fclose(fp);
    cv_reg[15] = 64;
    cv_run();
    for(i = 1; i < argc; i++)
        civetdisk_save(i-1, args[i]);
    return 0;
}
