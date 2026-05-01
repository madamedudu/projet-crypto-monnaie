#ifndef BLOC
#define BLOC
#include "define.h"

void finaliser_transaction_par_mineur(Transaction *tx, Account *donneur);
void lancer_phase_marche(Blockchain *bc, ListeAccounts *la, currency_t *curr_info);

#endif