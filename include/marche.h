#ifndef MARCHE_H
#define MARCHE_H

#include "define.h"

void creer_tx_coinbase(currency_t *currency, Block *bloc, Account *miner);
long calculer_recompense(Blockchain *bc) ;
long calculer_frais_bloc(Block *bloc);
long get_input_amount(Blockchain *bc, char *txHash, int index);
void finaliser_transaction_par_mineur(Transaction *tx, Account *donneur,ListeAccounts *la);
void lancer_phase_marche(Blockchain *bc, ListeAccounts *la, currency_t *curr_info);

#endif