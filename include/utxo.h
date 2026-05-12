#ifndef UTXO_H
#define UTXO_H
#include "define.h"

extern struct Slist *global_utxo_list;
void ajouter_utxo(TxOutputs *output, char *txid_source, int index);
void vider_liste_utxo(); 
TxOutputs* creer_output(long montant, char *nom_destinataire);
void afficher_utxo_global();
void supprimer_utxo(char *txid_source, int index);
Utxo* rechercher_utxo(char *txid_source, int index);
Slist* select_utxos_greedy(char *nom_emetteur, long montant_cible, long *somme_recuperee);
long calculer_solde_reel(struct account *acc);
#endif