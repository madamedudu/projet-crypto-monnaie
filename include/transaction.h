#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "define.h"

// ListeUsers generer_users(int nombre);
// int create_transaction(ListeUsers *liste_utilisateurs, Blockchain *bc, char *donneur, char *receuver, long montant);
Transaction* create_helicopter_transaction(char *dest_address, BYTE *dest_pub_key);
void run_helicopter_money(ListeAccounts *liste, currency_t *currency);

//ajouts phase 2
ListeAccounts generer_accounts(int nombre);
int create_transaction(ListeAccounts *liste_utilisateurs, Blockchain *bc, char *donneur, char *receuver, long montant);
Slist* inserer_en_tete(Slist *liste, void *data);
Slist* inserer_en_queue(Slist *liste, void *data);
Transaction* create_incomplete_transaction(Account *compte_donneur, char *nom_receveur, long montant);
void generer_transaction_aleatoire(ListeAccounts *liste_comptes);
struct account * trouver_compte(ListeAccounts *liste, char *nom);
struct account * trouver_compte_par_adresse(ListeAccounts *liste, char *address);
void liberer_accounts(ListeAccounts *liste);
void vider_mempool();
extern struct Slist *mempool;
Transaction* defiler_mempool();
void enfiler_mempool(Transaction *tx);

#endif