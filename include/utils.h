#ifndef UTILS_H
#define UTILS_H

#include "define.h"

void afficher_accounts(ListeAccounts liste);

void export_blockchain_json(ListeAccounts liste, Blockchain *blockchain, const char* filename);

#endif 