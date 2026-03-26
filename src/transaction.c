#include <stdio.h>
#include <string.h>
#include <time.h>
#include "transaction.h"
#include "define.h"

ListeUsers generer_users(int nombre) {
    ListeUsers liste;

    if(nombre <= 0 || nombre > MAX_USERS) {
        printf("Le nombre n'est pas valide\n");
        liste.users = NULL;
        liste.nb_users = 0;
        return liste;
    }

    liste.users = malloc(nombre * sizeof(User));
    if (liste.users == NULL) {
        printf("Erreur allocation mémoire\n");
        liste.nb_users = 0;
        return liste;
    }

    liste.nb_users = nombre;

    //pour chaque utilisateur, on lui donne un identifiant et on initialise son solde à 0
    for (int i = 0; i < nombre; i++) {
        snprintf(liste.users[i].adresse, 50, "USER_%d", i + 1);
        liste.users[i].solde = 0;
    }

    return liste;
} 

