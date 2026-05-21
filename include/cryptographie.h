#ifndef CRYPTOGRAPHIE_H
#define CRYPTOGRAPHIE_H

#include "define.h"
#include <stdint.h>

typedef unsigned char byte;

void calc_address(char address[35], char *pub_key_char);
void generer_cles(Account *compte);

#endif