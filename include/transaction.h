#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "define.h"

ListeUsers generer_users(int nombre);
Transaction create_helicopter_transaction(char *dest_address);
void run_helicopter_money(ListeUsers *liste, currency_t *currency);

#endif