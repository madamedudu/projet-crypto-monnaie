#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "define.h"
#include "sha256_utils.h"


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

    //--- creation de la chaîne de comparaison (ex: "0000" si difficulté = 4)
    char diff_str[difficulty + 1];

    for(int i = 0; i < difficulty; i++) {
        diff_str[i] = '0';
    }
    
    diff_str[difficulty] = '\0';

    printf("Minage du bloc en cours...\n");

    while (!success) { //"tant que on a pas trouver un bon hash"

        //--- créer une chaîne avec les infos du bloc + le Nonce actuel ---
        ssprintf(buffer, "%d%s%ld%s%ld", 
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

            printf("Bloc %d miné ! Hash: %s | Nonce: %ld\n", bloc->index, hash_res, bloc->nonce);} else {

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