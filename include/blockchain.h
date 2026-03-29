#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include "define.h"
void hash_transaction(Transaction trans, char hashRes[65]);
void hash_parent(char left[65], char right[65], char parentRes[65]);
void merkle_root(Transaction transactions[], int nb_transactions, char root[65]);
int verification_blockchain(Blockchain *bc);
int verification_merkle_bloc(Block *bloc);
int verification_merkle_blockchain(Blockchain *bc);
int verification_preuve_travail(Block *bloc,int difficulte);
int verification_hash_bloc(Block *bloc);
int ajouter_bloc_blockchain(Blockchain *bc, Block *nouveau_bloc, int difficulte);

#endif