#ifndef BLOC
#define BLOC
#include "define.h"
void mine_block(Block *bloc, int difficulty);
Block * create_genesis_block();
Blockchain * init_blockchain();
currency_t * init_currency();

#endif