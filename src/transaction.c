#include <stdio.h>
#include <string.h>
#include <time.h>
#include "transaction.h"
#include "bloc.h"
#include "define.h"

ListeUsers generer_users(int nombre) {
    ListeUsers liste;

    if(nombre <= 0 || nombre > MAX_USERS){
        printf("Le nombre n'est pas valide\n");
        liste.users = NULL;
        liste.nb_users = 0;
        return liste;
    }

    liste.users = malloc(nombre * sizeof(User));
    if (liste.users == NULL){
        printf("Erreur allocation mémoire\n");
        liste.nb_users = 0;
        return liste;
    }

    liste.nb_users = nombre;

    //pour chaque utilisateur, on lui donne un identifiant et on initialise son solde à 0
    for (int i = 0; i < nombre; i++){
        snprintf(liste.users[i].adresse, 50, "USER_%d", i + 1);
        liste.users[i].solde = 0;
    }

    return liste;
} 


int create_transaction(ListeUsers *liste_utilisateurs, Blockchain *bc, char *donneur, char *receuver, long montant){
    if (liste_utilisateurs == NULL || bc == NULL) return 0;


//-------------------------------GESTION USER----------------------------------------

    //On recherche les users dans la liste des users et on les stockent dans nos variables
    User *user_donne = NULL;
    User *user_recoit = NULL;
    for (int i = 0; i < liste_utilisateurs->nb_users; i++){
        if (strcmp(liste_utilisateurs->users[i].adresse, donneur) == 0) user_donne = &liste_utilisateurs->users[i];
        if (strcmp(liste_utilisateurs->users[i].adresse, receuver) == 0) user_recoit = &liste_utilisateurs->users[i];
    }

    if (user_donne == NULL || user_recoit == NULL){
        printf("L'utilisateur n'existe pas\n");
        return 0;
    }

    //On vérifie si l'utilisateur qui donne à assez de solde dans sa wallet
    if (user_donne->solde < montant){
        printf("L'utilisateur n'a pas assez de solde\n");
        return 0;
    }

    //On met a jour les wallets des deux users
    user_donne->solde -= montant;
    user_recoit->solde += montant;


//-------------------------------INFORMATION TRANSACTION----------------------------------------

    //On créer une transaction
    Transaction *transaction = malloc(sizeof(Transaction)); //on alloue la mémoire pour la transaction car elle sera stockée dans le bloc
    if (transaction == NULL) return 0;

    memset(transaction, 0, sizeof(Transaction)); //on met tout les champs de la transaction à 0

    //on remplit les information du donneur et receuver dans la structure de la transaction
    strncpy((char*)transaction->adSender, donneur, HASHLENGTH);
    strncpy((char*)transaction->adReceiver, receuver, HASHLENGTH);
    transaction->txAmount = montant;
    transaction->timestamp = time(NULL);

    // Calcul TXID
    char buffer[MAX_BUF];
    sprintf(buffer, "%s%s%ld%ld",            donneur, receuver, montant, transaction->timestamp); //on fabrique une chaine unique

    //On calcule le Hash (c'est l'identifiant de la transaction)
    sha256ofString((BYTE *)buffer, (char*)transaction->txid);


//-------------------------------INSERTION DANS LE BLOC----------------------------------------

    //On trouve le dernier bloc
    Slist *bloc_courant = bc->blocklist;
    while (bloc_courant->next != NULL){
        bloc_courant = bloc_courant->next;
    }
        
    Block *dernier_bloc = (Block*)bloc_courant->info; //on stocke le dernier bloc dans une variable dernier_bloc

    //on creer un nouveau noeud dans la liste chainée qui contient la transaction
    Slist *nouveau_noeud_transaction = malloc(sizeof(Slist)); 
    nouveau_noeud_transaction->info = transaction;
    nouveau_noeud_transaction->next = NULL;

    if (dernier_bloc->transactions == NULL){ //si le bloc est vide
        dernier_bloc->transactions = nouveau_noeud_transaction;
    } 
    else{ //si le bloc n'est pas vide, on ajoute a la fin
        Slist *temp = dernier_bloc->transactions;
        while (temp->next != NULL){
            temp = temp->next;
        }
        temp->next = nouveau_noeud_transaction;
    }

    //On incrémente le nombre de tranactions
    dernier_bloc->nbTx++;

    printf("Transaction ajoutee ! : %s -> %s : %ld BT\n", donneur, receuver, montant);

    //Si on atteinds 10 transactions alors on fait le minage et on creer un nouveau bloc pour les futures transactions
    if (dernier_bloc->nbTx == MAXTX) {
        printf("Minage du bloc...\n");
        mine_block(dernier_bloc, bc->difficulty);
        create_nouveau_bloc(bc);
    }

    return 1;
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

