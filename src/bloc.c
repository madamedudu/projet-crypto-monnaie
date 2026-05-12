#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "define.h"
#include "sha256_utils.h"
#include "blockchain.h"
 
//modif par chirine pour empecher que le nv bloc créé rentre dans la blockchain sans verif
Block* create_nouveau_bloc(Blockchain *bc) {
    if (bc == NULL || bc->blocklist == NULL) return NULL;
    
    //On trouve le dernier bloc
    Slist *bloc_courant = bc->blocklist;
    while (bloc_courant->next != NULL){
        bloc_courant = bloc_courant->next;
    }

    Block *dernier_bloc = (Block *)bloc_courant->info;

    //On créer un nouveau bloc
    Block *nouveau_block = malloc(sizeof(Block));
    if (nouveau_block == NULL){
        printf("erreur allocation du nouveau bloc\n");
        return NULL;
    }
 
    memset(nouveau_block, 0, sizeof(Block)); //On met les champs un bloc à 0 au debut

    //On initialise le nouveau bloc
    nouveau_block->index = bc->nbBlocks;
    nouveau_block->timestamp = time(NULL);
    nouveau_block->nbTx = 0;
    nouveau_block->transactions = NULL;
    nouveau_block->nonce = 0;

    //On relie le nouveau bloc avec le dernier bloc de la blockchain
    strcpy((char *)nouveau_block->previousHash, (char *)dernier_bloc->blockHash);

    //On initialise des champs de texte
    memset(nouveau_block->merkleTree, '0', HASHLENGTH - 1);
    nouveau_block->merkleTree[HASHLENGTH - 1] = '\0';

    memset(nouveau_block->blockHash, 0, HASHLENGTH);

    strcpy(nouveau_block->minerName, "");
    strcpy(nouveau_block->comment, "New pending block");

    return nouveau_block;
}


//--- mining (hash) ---
void mine_block(Block *bloc, int difficulty) {

    char buffer[MAX_BUF]; // pour concatener les donnees du bloc
    char hash_res[HASHLENGTH]; //stocker le resultat du hashage SHA256 en hexadécimal.
    /* 
    HASHLENGTH :
    ' SHA256_BLOCK_SIZE ' = la taille donne dans le fichier sha256.h
    ' *2 ' = on multiplie fois 2 car chaque octet binaire a c'est 2 caracteres hexadecimal 
    ' +1 ' = on ajoute 1 pour '\0'
    */

    int success = 0; //pour touver le hash valide (difficulty = 4)


   // --- Calcul du Merkle Root si on a des transactions ---
    // Si c'est le bloc Genesis, nbTx vaut 0 donc on saute cette étape.
    if (bloc->nbTx > 0 && bloc->transactions != NULL) {
        
        // CORRECTION ICI : On passe directement la Slist* (bloc->transactions) !
        // Plus besoin de malloc, de boucle, ni de free.
        merkle_root(bloc->transactions, bloc->nbTx, (char*)bloc->merkleTree);
        
        printf("Arbre de Merkle calcule : %s\n", bloc->merkleTree);
    }

    //--- creation de la chaîne de comparaison (ex: "0000" si difficulté = 4)
    char diff_str[difficulty + 1];

    for(int i = 0; i < difficulty; i++) {
        diff_str[i] = '0';
    }
    
    diff_str[difficulty] = '\0';

    printf("Minage du bloc %d en cours...\n", bloc->index);

    while (!success) { //"tant que on a pas trouver un bon hash"

        //--- créer une chaîne avec les infos du bloc + le Nonce actuel ---
        sprintf(buffer, "%d%s%ld%s%ld", 
                bloc->index,
                (char*)bloc->previousHash, 
                (long)bloc->timestamp, 
                (char*)bloc->merkleTree, 
                bloc->nonce);

        //--- calculer le hash de notre buffer (en hexadécimal)
        sha256ofString((BYTE *)buffer, hash_res);

        //--- verifier si le hash commence par "0000" (difficulty = 4)
        // comparerles 4 premiers caracteres de la chaîne hexadécimale
        if (strncmp(hash_res, diff_str, difficulty) == 0) {
            success = 1; //hash trouvé commence par "0000"

            // On enregistre le hash trouve dans le bloc
            strncpy((char*)bloc->blockHash, hash_res, HASHLENGTH);

            printf("Bloc %d miné ! Hash: %s | Nonce: %ld\n", bloc->index, hash_res, bloc->nonce);
        } else {

            bloc->nonce++; // On incremente et on recommence
        }
    }
}


//--- creation du block genesis ---
Block * create_genesis_block(){

    //--- allocation du memoire pour le bloc ---
    Block * genesis = malloc(sizeof(Block));
    if(genesis == NULL){
        perror("allocation du block");
        return NULL;
    }

    //--- initialisation du hash a 0 car c'est le bloc genesis (format hexadécimal) ---
    //memset(adresse, valeur, taille) on force la 'valeur' a l'@ 'adresse' de taille 'taille'
    /* a la place de memset on peut faire la boucle, mais c'est plus longue
    for (int i = 0; i < HASH_SIZE; i++) {
    genesis->previousHash.HashValue[i] = 0;
    }
    */
    memset(genesis->previousHash, '0', HASHLENGTH - 1);
    genesis->previousHash[HASHLENGTH - 1] = '\0';

    //--- pas de transactions ---
    memset(genesis->merkleTree, '0', HASHLENGTH - 1);
    genesis->merkleTree[HASHLENGTH - 1] = '\0';

    //--- initialisation des autres parametres ---
    genesis->timestamp = time(NULL); //renvoi le temps en secondes
    genesis->nbTx = 0; //nbr des transactions
    genesis->transactions = NULL; //liste de transactions
    genesis->nonce = 0;
    
    strncpy(genesis->minerName, "System-Coinbase", MAX_STRING);  //car c'est le genesis
    strncpy(genesis->comment, "Genesis Block - Welcome", MAX_STRING);

    //--- on mine le bloc pour remplir le hash ---
    mine_block(genesis, DIFFICULTY);

    return genesis;
}

//--- initialisation struct Blockchain ---
Blockchain * init_blockchain() {

    //--- allocation du blockchain ---
    Blockchain *blockchain = (Blockchain *)malloc(sizeof(Blockchain));
    if (blockchain == NULL){
        perror("allocation du blockchain");
        return NULL;
    }

    blockchain->difficulty = DIFFICULTY;
    blockchain->reward4mining = INITIALREWARD; // Défini dans defines.h

    //--- creation du bloc Genesis ---
    Block *genesis = create_genesis_block();
    
    //--- creation du premier block de la liste Slist
    Slist *element = (Slist *)malloc(sizeof(Slist));

    if (element != NULL && genesis != NULL) {
        element->info = (void *)genesis; // On stocke le bloc dans 'info'
        element->next = NULL;
        
        blockchain->blocklist = element; 
        blockchain->nbBlocks = 1;
    }

    return blockchain;
}


//--- initialisation de la monnaie (struct currency_t) ---
currency_t * init_currency() {

    //--- allocation struct currency ---
    currency_t * currency = (currency_t *)malloc(sizeof(currency_t));
    if (currency == NULL){
        perror("allocation currency");
        return NULL;
    }

    //yayy = le nom a changer
    strncpy(currency->currency_name, "BTU", MAX_STRING);
    currency->moneySupply = 0; // Sera mis à jour lors de l'helicopter money
    
    //--- initialisation du blockchain ---
    currency->bc = init_blockchain();

    printf("Monnaie '%s' initialisée avec succès.\n", currency->currency_name);
    return currency;
}