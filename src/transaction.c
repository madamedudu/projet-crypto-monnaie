#include <stdio.h>
#include <string.h>
#include <time.h>
#include "transaction.h"
#include "bloc.h"
#include "define.h"
#include "utxo.h"
#include "cryptographie.h"

// ListeUsers generer_users(int nombre) {
//     ListeUsers liste;

//     if(nombre <= 0 || nombre > MAX_USERS){
//         printf("Le nombre n'est pas valide\n");
//         liste.users = NULL;
//         liste.nb_users = 0;
//         return liste;
//     }

//     liste.users = malloc(nombre * sizeof(User));
//     if (liste.users == NULL){
//         printf("Erreur allocation mémoire\n");
//         liste.nb_users = 0;
//         return liste;
//     }

//     liste.nb_users = nombre;

//     //pour chaque utilisateur, on lui donne un identifiant et on initialise son solde à 0
//     for (int i = 0; i < nombre; i++){
//         snprintf(liste.users[i].adresse, 50, "USER_%d", i + 1);
//         liste.users[i].solde = 0;
//     }

//     return liste;
// } 




// Fonction utilitaire
Slist* inserer_en_tete(Slist *liste, void *data) {
    Slist *nouveau = malloc(sizeof(Slist));
    if (!nouveau) return liste;
    nouveau->info = data;
    nouveau->next = liste;
    return nouveau;
}










//------------------------------MODIFICATIONS CHIRINE PHASE 2------------------------------

//fonction pour générer une liste d'utilisateurs mais de struct type  ACCOUNT
ListeAccounts generer_accounts(int nombre) {
    ListeAccounts liste;

    if(nombre <= 0 || nombre > MAX_USERS){
        printf("Le nombre n'est pas valide\\n");
        liste.accounts = NULL;
        liste.nb_accounts = 0;
        return liste;
    }

    liste.accounts = malloc(nombre * sizeof(struct account));
    if (liste.accounts == NULL){
        printf("Erreur allocation mémoire\\n");
        liste.nb_accounts = 0;
        return liste;
    }

    liste.nb_accounts = nombre;

    //pour chaque utilisateur, on lui donne un identifiant et on initialise son solde à 0
    for (int i = 0; i < nombre; i++){
        // On utilise les champs 'str' et 'balance' de la struct du prof
        snprintf(liste.accounts[i].str, 50, "USER_%d", i + 1);
        liste.accounts[i].balance = 0;
        liste.accounts[i].utxoList = NULL; // Liste UTXO vide au depart
        generer_cles(&liste.accounts[i]);
    }

    return liste;
}









int create_transaction(ListeAccounts *liste_utilisateurs, Blockchain *bc, char *donneur, char *receuver, long montant){
    if (liste_utilisateurs == NULL || bc == NULL) return 0;


//-------------------------------GESTION USER----------------------------------------

    //On recherche les users dans la liste des users et on les stockent dans nos variables
    struct account *account_donne = NULL;
    struct account *account_recoit = NULL;
    
    // On cherche avec .str au lieu de .adresse
    for (int i = 0; i < liste_utilisateurs->nb_accounts; i++){
        if (strcmp(liste_utilisateurs->accounts[i].str, donneur) == 0) account_donne = &liste_utilisateurs->accounts[i];
        if (strcmp(liste_utilisateurs->accounts[i].str, receuver) == 0) account_recoit = &liste_utilisateurs->accounts[i];
    }
    //modif user -> account
    if (account_donne == NULL || account_recoit == NULL){
        printf("L'utilisateur n'existe pas\n");
        return 0;
    }

    // On applique le taux de 5% défini dans define.h (Zyad)
    long frais = (montant * FEE_RATE) / 100; 
    long total_a_payer = montant + frais;
    long somme_accumulee = 0;

    //On vérifie si l'utilisateur qui donne à assez de solde dans sa wallet
    //modif user -> account
    //ajout algorithme glouton pour sélectionner les UTXO à utiliser pour la transaction
    
    Slist *utxos_choisis = select_utxos_greedy(account_donne->str, total_a_payer, &somme_accumulee);
    if (utxos_choisis == NULL) {
        printf("L'utilisateur %s n'a pas assez de solde (Besoin total: %ld BT)\n", donneur, total_a_payer);
        return 0;
    }

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
    sprintf(buffer, "%s%s%ld%ld", donneur, receuver, montant, transaction->timestamp); //on fabrique une chaine unique

    //On calcule le Hash (c'est l'identifiant de la transaction)
    sha256ofString((BYTE *)buffer, (char*)transaction->txid);

    //-------------SIGNATURE--------------------
    BYTE signature[HASHLENGTH];
    signer_transaction(transaction, account_donne->priv_key, signature);

    //---------------- INPUT ---------------- (modife Zyad)
    Slist *curr = utxos_choisis;
    while (curr != NULL) {
        Utxo *u = (Utxo*)curr->info;
        
        TxInputs *input = malloc(sizeof(TxInputs));
        memset(input, 0, sizeof(TxInputs));
        
        strcpy((char*)input->txHash, (char*)u->hash);
        input->indexOutput = u->indexOutput;
        
        // On déverrouille le billet avec la signature
        creer_unlock_script(input, signature, (BYTE*)donneur);

        // Ajout à la liste des entrées de la transaction
        transaction->lstInputs = inserer_en_tete(transaction->lstInputs, input);
        transaction->nbInputs++;

        // On retire le billet du registre global puisqu'il est utilisé
        supprimer_utxo((char*)u->hash, u->indexOutput);
        
        curr = curr->next;
    }

    //---------------- OUTPUT ---------------- (modife Zyad)
    // Le paiement pour le destinataire
    TxOutputs *out_dest = creer_output(montant, receuver);
    transaction->lstOutputs = inserer_en_tete(NULL, out_dest);
    transaction->nbOutputs = 1;
    ajouter_utxo(out_dest, (char*)transaction->txid, 0);

    // Le change (Monnaie rendue au donneur)
    long monnaie_rendue = somme_accumulee - total_a_payer;
    if (monnaie_rendue > 0) {
        TxOutputs *out_change = creer_output(monnaie_rendue, donneur);
        transaction->lstOutputs = inserer_en_tete(transaction->lstOutputs, out_change);
        transaction->nbOutputs++;
        
        // Le donneur récupère ce nouveau billet dans son portefeuille
        ajouter_utxo(out_change, (char*)transaction->txid, 1);
    }

//-----------------------------MISE A JOUR DES SOLDES-------------------------------------- (modife Zyad)

    //On met a jour les wallets des deux users 
    account_donne->balance -= total_a_payer; //modif user -> account
    account_recoit->balance += montant; //modif user -> account


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
    //libération de la mémoire pour la liste des UTXO choisis
    while (utxos_choisis != NULL) {
        Slist *temp = utxos_choisis;
        utxos_choisis = utxos_choisis->next;
        free(temp); 
    }

    return 1;
}







/**
 * Créer la transaction d'Helicopter Money
 * Adapte l'adresse de destination en fonction de la structure User
 * Distribue le montant initial à tous les utilisateurs.
 */

//---------Modification Chirine---------
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

//à finir ici
    // initialisation : phase 2
    trans.nbInputs = 0;
    trans.lstInputs = NULL;
    trans.nbOutputs = 1;
    //fonction pour créer output
    TxOutputs *nouveau_output = creer_output(HELIREWARD, dest_address);
    if (nouveau_output != NULL) {
        //nœud Slist pour la liste lstOutputs de la transaction
        struct Slist *noeud = malloc(sizeof(struct Slist));
        if (noeud != NULL) {
            noeud->info = (void *)nouveau_output;
            noeud->next = NULL;
            //le billet dans la liste des sorties de la transaction
            trans.lstOutputs = noeud;
        } else {
            printf("[Erreur] Echec malloc noeud transaction.\n");
    
        }
    }
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




void run_helicopter_money(ListeAccounts *liste, currency_t *currency) {
    if (liste == NULL || currency == NULL) return;
    printf(" Lancement : Helicopter Money (Phase 2 UTXO)\n");
    
    //on parcours toute la liste cree 
    for (int i = 0; i < liste->nb_accounts; i++) {
        
    
        Transaction h_trans = create_helicopter_transaction(liste->accounts[i].str);
         if (h_trans.lstOutputs != NULL) {
            TxOutputs *out = (TxOutputs *)h_trans.lstOutputs->info;
            ajouter_utxo(out, (char*)h_trans.txid, 0); 
        
            //MAJ des compteurs pour l'affichage et la simulation
            liste->accounts[i].balance += h_trans.txAmount;
            currency->moneySupply += h_trans.txAmount;
            
            printf("[Coinbase] Output cree pour %s (TXID court: %.8s)\n", 
                   liste->accounts[i].str, (char*)h_trans.txid);
 
            
            //on ne libère PAS ->info (le TxOutputs) car il appartient à global_utxo_list
            free(h_trans.lstOutputs);
            h_trans.lstOutputs = NULL;
        }
    }
    
    printf("fin : Helicopter Money\n");
    printf("Monney Supply total : %ld \n", currency->moneySupply);
}


// Crée une transaction incomplète (Emetteur, Destinataire, Montant, Inputs)
Transaction* create_incomplete_transaction(Account *compte_donneur, char *nom_receveur, long montant) {
    long accumule = 0;

    // On sélectionne les billets (Inputs) via le glouton
    Slist *inputs_choisis = select_utxos_greedy(compte_donneur->str, montant, &accumule);
    
    if (inputs_choisis == NULL) {
        return NULL; // Fonds insuffisants
    }

    //On alloue et on remplit la transaction
    Transaction *tx = malloc(sizeof(Transaction));
    memset(tx, 0, sizeof(Transaction));

    strncpy((char*)tx->adSender, compte_donneur->str, HASHLENGTH);
    strncpy((char*)tx->adReceiver, nom_receveur, HASHLENGTH);
    tx->txAmount = montant;

    tx->lstInputs = inputs_choisis;
    
    // On laisse txid, timestamp et lstOutputs vides pour le mineur
    return tx;
}

// Générateur aléatoire de transactions 
void generer_transaction_aleatoire(ListeAccounts *liste_comptes) {
    if (liste_comptes == NULL || liste_comptes->nb_accounts < 2){ 
        return;
    }

    // Choisir deux utilisateurs différents au hasard
    int idx_donneur = rand() % liste_comptes->nb_accounts;
    int idx_receveur;
    do {
        idx_receveur = rand() % liste_comptes->nb_accounts;
    } while (idx_donneur == idx_receveur);

    Account *donneur = &liste_comptes->accounts[idx_donneur];
    char *receveur = liste_comptes->accounts[idx_receveur].str;

    // Choisir un montant aléatoire
    long montant = (rand() % 100) + 1;

    // Créer la transaction incomplète
    Transaction *tx = create_incomplete_transaction(donneur, receveur, montant);

    if (tx != NULL) {
        printf("Nouvelle transaction créée : %s envoie %ld BT à %s\n", 
               donneur->str, montant, receveur);
    } else {
        printf("Échec : %s n'a pas assez de fonds pour %ld BT\n", 
               donneur->str, montant);
    }
}