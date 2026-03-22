#include <stdio.h>
#include <string.h>
#include <time.h>
#include "transaction.h"
#include "define.h"

void generer_user(User *user, int nombre){
    for(int i=0; i<nombre; i++){
        snprintf(user[i].adresse, ADRESS_SIZE, "USER_%d", i); //on remplit le champ adresse avec le numéro i (id)
        user[i].solde = 0.0; //au début le solde d'un user est à 0
    }
}

