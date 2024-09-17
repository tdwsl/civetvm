#include "civetvm.h"
#include "civetdisk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CVDISKSZ (2*1024*1024)
#define CVDISKMAX 4

static void diska(int op);
static void diskb(int op);
static void diskc(int op);
static void diskd(int op);

static void (*diskf[])(int) = { diska, diskb, diskc, diskd, };
unsigned ndisks = 0;
static char change = 0;

static char *disks[CVDISKMAX];
static unsigned curs[CVDISKMAX];

/* disk flags:
 * ---- --wr
 * r - disk is connected, read operations will work
 * w - write access to disk, 0 for read-only
 */

static unsigned filled(unsigned d) {
    unsigned i;
    for(i = CVDISKSZ-1; i && !disks[d][i]; i--);
    if(disks[d][i]) i++;
    return i;
}

static void civetdisk(int d, int op) {
    unsigned i;
    switch(op) {
    case 0: /* disk status - see flags */
        cv_reg[1] = 3;
        break;
    case 1: /* max disk CVDISKSZ - same for all disks */
        cv_reg[1] = CVDISKSZ;
        break;
    case 2: /* last point of data on disk, fileCVDISKSZ of disk image */
        cv_reg[1] = filled(d);
        break;
    case 3: /* seek */
        curs[d] = cv_reg[1];
        break;
    case 5: /* read, r1 = bytes read */
        for(i = 0; curs[d] < CVDISKSZ && i < cv_reg[2]; i++ + curs[d]++)
            cv_sb(cv_reg[1]+i, disks[d][curs[d]]);
        cv_reg[1] = i;
        break;
    case 6: /* write, r1 = bytes written */
        for(i = 0; curs[d] < CVDISKSZ && i < cv_reg[2]; i++ + curs[d]++)
            disks[d][curs[d]] = cv_lb(cv_reg[1]+i);
        cv_reg[1] = i;
        if(i) change |= 1<<d;
        break;
    }
}

static void diska(int op) { civetdisk(0, op); }
static void diskb(int op) { civetdisk(1, op); }
static void diskc(int op) { civetdisk(2, op); }
static void diskd(int op) { civetdisk(3, op); }


unsigned connect_civetdisk() {
    if(ndisks+1 >= CVDISKMAX) { printf("too many disks\n"); exit(1); }
    disks[ndisks] = malloc(CVDISKSZ);
    cv_connect(diskf[ndisks], 0x6b736964);
    return ndisks++;
}

void civetdisk_load(unsigned d, const char *filename) {
    FILE *fp;
    unsigned r;
    if(d >= ndisks) { printf("disk drive out of range\n"); }
    fp = fopen(filename, "rb");
    if(!fp) { printf("failed to open disk %s\n", filename); exit(1); }
    r = fread(disks[d], 1, CVDISKSZ, fp);
    memset(disks[d]+r, 0, CVDISKSZ-r);
    fclose(fp);
}

void civetdisk_save(unsigned d, const char *filename) {
    FILE *fp;
    if(d >= ndisks) { printf("disk drive out of range\n"); }
    if(!(change&(1<<d))) return;
    while(!(fp = fopen(filename, "wb"))) {
        printf("failed to open disk %s\n", filename);
        printf("press return to try again");
        while(fgetc(stdin) != '\n');
    }
    fwrite(disks[d], 1, filled(d), fp);
    fclose(fp);
}

