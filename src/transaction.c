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


/**
 * Créer la transaction d'Helicopter Money
 * Adapte l'adresse de destination en fonction de la structure User
 * Distribue le montant initial à tous les utilisateurs.
 */
Transaction create_helicopter_transaction(char *dest_address) {
    Transaction trans;

    //sender : système (Coinbase)
    memset(trans.adSender, 0, HASHLENGTH);
    strncpy((char*)trans.adSender, "SYSTEM_COINBASE", HASHLENGTH);
    
    // destination : utilisateur
    memset(trans.adReceiver, 0, HASHLENGTH);
    strncpy((char*)trans.adReceiver, dest_address, HASHLENGTH);

    // initialisation d'autres transactions
    trans.txAmount = HELIREWARD; 
    trans.timestamp = time(NULL);
    strncpy(trans.comment, "Helicopter Money", MAX_STRING);

    // initialisation : phase 2
    trans.nbInputs = 0;
    trans.lstInputs = NULL;
    trans.nbOutputs = 0;
    trans.lstOutputs = NULL;

    // calcul : TXID (hash de la transaction)
    char buffer[MAX_BUF];
    sprintf(buffer, "%s%s%ld%ld", 
            (char*)trans.adSender, 
            (char*)trans.adReceiver, 
            trans.txAmount, 
            trans.timestamp);
    
    sha256ofString((BYTE *)buffer, (char*)trans.txid);

    return trans;
}


/**
 * phase globale d'Helicopter Money
 * Utilise la ListeUsers (ListeUsers generer_users(int nombre)) et met à jour la masse monétaire.
 */
void run_helicopter_money(ListeUsers *liste, currency_t *currency) {
    printf(" Lancement : Helicopter Money\n");
    
    // on parcours toute la liste cree par la fct : ListeUsers generer_users(int nombre)
    for (int i = 0; i < liste->nb_users; i++) {
        
        // recu (la transaction)
        Transaction h_trans = create_helicopter_transaction(liste->users[i].adresse);
        
        // on met à jour le solde (en utilisant le champ .solde de sa struct)
        liste->users[i].solde += h_trans.txAmount;
        
        // on augmente la monnaySupply (l'argent vient d'être créé)
        currency->moneySupply += h_trans.txAmount;
        
        printf("[Coinbase] %d BT envoyés à %s (Nouveau solde: %ld)\n", 
                HELIREWARD, liste->users[i].adresse, (long)liste->users[i].solde);
    }
    
    printf("fin : Helicopter Money\n");
    printf("Monney Supply total : %ld \n", currency->moneySupply);
}

