#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "define.h"

// ListeUsers generer_users(int nombre);
// int create_transaction(ListeUsers *liste_utilisateurs, Blockchain *bc, char *donneur, char *receuver, long montant);
Transaction create_helicopter_transaction(char *dest_address);
void run_helicopter_money(ListeAccounts *liste, currency_t *currency);

//ajouts phase 2
ListeAccounts generer_accounts(int nombre);
int create_transaction(ListeAccounts *liste_utilisateurs, Blockchain *bc, char *donneur, char *receuver, long montant);

#endif