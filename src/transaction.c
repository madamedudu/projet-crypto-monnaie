#include <stdio.h>
#include <string.h>
#include <time.h>
#include "transaction.h"
#include "bloc.h"
#include "define.h"
#include "utxo.h"
#include "cryptographie.h"
#include "blockchain.h"

//pour l'ordre des outputs
Slist* inserer_en_tete(Slist *liste, void *data) {
    Slist *nouveau = malloc(sizeof(Slist));
    if (!nouveau) return liste;
    nouveau->info = data;
    nouveau->next = liste;
    return nouveau;
}
Slist* inserer_en_queue(Slist *liste, void *data) {
    Slist *nouveau = malloc(sizeof(Slist));
    if (!nouveau) {
        return liste;
    }
    nouveau->info = data;
    nouveau->next = NULL;

    if (liste == NULL) {
        return nouveau;
    }

    Slist *courant = liste;
    while (courant->next != NULL) courant = courant->next;
    courant->next = nouveau;
    return liste;
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




/**
 * Créer la transaction d'Helicopter Money
 * Adapte l'adresse de destination en fonction de la structure User
 * Distribue le montant initial à tous les utilisateurs.
 */

//---------Modification Chirine---------
//change des listes par accounts
// modifs Kasia
// la fct doit mtn accepter la clé publique du destinataire dest_pub_key
// creer_output doit intégrer clé publique
Transaction* create_helicopter_transaction(char *dest_address, BYTE *dest_pub_key) {
    Transaction *trans = malloc(sizeof(Transaction));
    memset(trans, 0, sizeof(Transaction));

    //sender : système (Coinbase)
    strncpy((char*)trans->adSender, "SYSTEM_COINBASE", ADDRESS_LEN);
    strncpy((char*)trans->adReceiver, dest_address, ADDRESS_LEN);
    trans->txAmount = HELIREWARD; 
    trans->timestamp = time(NULL);
    strncpy(trans->comment, "Helicopter Money", MAX_STRING);

    trans->nbInputs = 0;
    trans->lstInputs = NULL;
    trans->nbOutputs = 1;
    
    TxOutputs *nouveau_output = creer_output(HELIREWARD, dest_address, dest_pub_key);

    if (nouveau_output != NULL) {
        struct Slist *noeud = malloc(sizeof(struct Slist));
        noeud->info = (void *)nouveau_output;
        noeud->next = NULL;
        trans->lstOutputs = noeud;
    }

    char buffer[MAX_BUF];
    sprintf(buffer, "%s%s%ld%ld", 
            (char*)trans->adSender, 
            (char*)trans->adReceiver, 
            trans->txAmount, 
            trans->timestamp);
    
    sha256ofString((BYTE *)buffer, (char*)trans->txid);

    return trans;
}



//modifs Kasia
//passer la vraie adresse Base58 et la vraie clé publique du compte.
void run_helicopter_money(ListeAccounts *liste, currency_t *currency) {
    if (liste == NULL || currency == NULL) return;
    printf(" Lancement : Helicopter Money (Phase 2 UTXO)\n");
    
    //on parcours toute la liste cree 
    for (int i = 0; i < liste->nb_accounts; i++) {
        
    
        //T8: on utilise address et pub_key du compte
        Transaction *h_trans = create_helicopter_transaction((char*)liste->accounts[i].address, liste->accounts[i].pub_key);        
        //on crée un bloc temporaire hors de la chaîne
        Block *bloc_a_miner = create_nouveau_bloc(currency->bc);
        if (bloc_a_miner == NULL) break;
        strncpy(bloc_a_miner->minerName, "System-Coinbase", MAX_STRING);
        strncpy(bloc_a_miner->comment, "Helicopter Money", MAX_STRING);

        //on ajoute la transaction directement au bloc temporaire
        bloc_a_miner->transactions = inserer_en_tete(bloc_a_miner->transactions, h_trans);
        bloc_a_miner->nbTx++;

         if (h_trans->lstOutputs != NULL) {
            TxOutputs *out = (TxOutputs *)h_trans->lstOutputs->info;
            ajouter_utxo(out, (char*)h_trans->txid, 0); 
        
            //MAJ des compteurs pour l'affichage et la simulation
            liste->accounts[i].balance += h_trans->txAmount;
            currency->moneySupply += h_trans->txAmount;
            
            printf("[Coinbase] Output cree pour %s (TXID court: %.8s)\n", 
                   liste->accounts[i].str, (char*)h_trans->txid);
 
        }    
         // On mine ce bloc immédiatement pour qu'il ne contienne QUE cette transaction (PDF Req)
        mine_block(bloc_a_miner, currency->bc->difficulty);
        
        //on ajoute à la chaîne avec vérifications
        if (!ajouter_bloc_blockchain(currency->bc, bloc_a_miner, currency->bc->difficulty)) {
            printf("[Erreur] Bloc helicopter money rejeté.\n");
            free(bloc_a_miner);
        }
        
    }
    
    printf("fin : Helicopter Money\n");
    printf("Monney Supply total : %ld \n", currency->moneySupply);
}

// Crée une transaction incomplète (Emetteur, Destinataire, Montant, Inputs)
Transaction* create_incomplete_transaction(Account *compte_donneur, char *nom_receveur, long montant) {
    long accumule = 0;

    // On sélectionne les billets (Inputs) via le glouton
    long frais = (montant * FEE_RATE) / 100;
    long total_a_payer = montant + frais;
    Slist *inputs_choisis = select_utxos_greedy(compte_donneur->str, total_a_payer, &accumule);
    
    if (inputs_choisis == NULL) {
        return NULL; // Fonds insuffisants
    }

    //On alloue et on remplit la transaction
    Transaction *tx = malloc(sizeof(Transaction));
    memset(tx, 0, sizeof(Transaction));

    strncpy((char*)tx->adSender, compte_donneur->str, ADDRESS_LEN);
    strncpy((char*)tx->adReceiver, nom_receveur, ADDRESS_LEN);
    tx->txAmount = montant;
    tx->lstInputs = inputs_choisis;
    
    //comptage des inputs
    Slist *tmp = inputs_choisis;
    tx->nbInputs = 0;
    while (tmp != NULL) {
        tx->nbInputs++;
        tmp = tmp->next;
    }
    // On laisse txid, timestamp et lstOutputs vides pour le mineur
    return tx;
}

//fifo la liste d'attente des transaction qui sont empilé puis dépilé
struct Slist *mempool = NULL;
void enfiler_mempool(Transaction *tx) {
    if (tx == NULL) {
        return;
    }

    Slist *nouveau_noeud = malloc(sizeof(Slist));
    if (nouveau_noeud == NULL) {
        return;
    }
    
    nouveau_noeud->info = tx;
    nouveau_noeud->next = NULL;

    if (mempool == NULL) {
        mempool = nouveau_noeud;
    } else {
        Slist *courant = mempool;
        while (courant->next != NULL) {
            courant = courant->next;
        }
        courant->next = nouveau_noeud;
    }
}
Transaction* defiler_mempool() {
    if (mempool == NULL) {
        return NULL; // La file est vide
    }

    Slist *noeud_a_supprimer = mempool;
    Transaction *tx = (Transaction*)noeud_a_supprimer->info;
    
    mempool = mempool->next; //2ème éléme devient le premier
    free(noeud_a_supprimer); //  libère le noeud de la liste
    
    return tx;
}

// Générateur aléatoire de transactions pour la phase de marché
void generer_transaction_aleatoire(ListeAccounts *liste_comptes) {
    if (liste_comptes == NULL || liste_comptes->nb_accounts < 2){ 
        return;
    }

    //deux utilisateurs diffé au hasard
    int idx_donneur = rand() % liste_comptes->nb_accounts;
    int idx_receveur;
    do {
        idx_receveur = rand() % liste_comptes->nb_accounts;
    } while (idx_donneur == idx_receveur);

    Account *donneur = &liste_comptes->accounts[idx_donneur];
    char *receveur = liste_comptes->accounts[idx_receveur].str;

    //montant aléatoire
    long montant = (rand() % 100) + 1;

    //transaction incomplète
    Transaction *tx = create_incomplete_transaction(donneur, receveur, montant);
    if (tx != NULL) {

        enfiler_mempool(tx);
        printf("\tTX ajoutée au Memory pool : %s -> %s (%ld BTU)\n", donneur->str, receveur, montant);
    } else {
        printf("Échec : %s n'a pas assez de fonds pour envoyer %ld BTU.\n", donneur->str, montant);
    }
}

//fonction pour l'ajout manuel d'une transaction verifie si l'utilisateur est dans la liste
struct account * trouver_compte(ListeAccounts *liste, char *nom){
    if (liste==NULL || nom==NULL) {
        return NULL;
    }
    //on va dans une boucle 
    for (int i=0; i< liste->nb_accounts; i++) {
        if (strcmp(liste->accounts[i].str, nom) == 0) {
            return &liste->accounts[i];
        }
    }
    return NULL;
}

void liberer_accounts(ListeAccounts *liste){
    if(liste == NULL || liste->accounts == NULL) return;

    free(liste->accounts);

    liste->accounts = NULL;
    liste->nb_accounts = 0;
}