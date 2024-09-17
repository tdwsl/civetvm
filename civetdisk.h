#ifndef CIVETDISK_H
#define CIVETDISK_H

unsigned connect_civetdisk();
void civetdisk_load(unsigned d, const char *filename);
void civetdisk_save(unsigned d, const char *filename);

#endif
