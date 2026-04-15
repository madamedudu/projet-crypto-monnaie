#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "define.h"

ListeUsers generer_users(int nombre);
int create_transaction(ListeUsers *liste_utilisateurs, Blockchain *bc, char *donneur, char *receuver, long montant);
Transaction create_helicopter_transaction(char *dest_address);
void run_helicopter_money(ListeUsers *liste, currency_t *currency);

#endif