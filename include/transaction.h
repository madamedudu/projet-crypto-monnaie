#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "define.h"

//ListeUsers generer_user(User *user, int nombre); (zizu): J'ai corrige l'appel de la fonction)
ListeUsers generer_users(int nombre);
void run_helicopter_money(ListeUsers *liste, currency_t *currency);
Transaction create_helicopter_transaction(char *dest_address);
#endif