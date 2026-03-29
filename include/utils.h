#ifndef UTILS_H
#define UTILS_H

#include "define.h"

void afficher_users(ListeUsers liste);

void export_blockchain_json(ListeUsers liste, Blockchain *bc, const char* filename);

#endif 