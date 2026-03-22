#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "define.h"
#include "sha256_utils.h"

void mine_block(Bloc *bloc) {

    char buffer[1000]; // pour concatener les donnees du bloc
    char hash_res[SHA256_BLOCK_SIZE * 2 + 1]; //stocker le resultat du hashage SHA256 en hexadécimal.
    /* 
    ' SHA256_BLOCK_SIZE ' = la taille donne dans le fichier sha256.h
    ' *2 ' = on multiplie fois 2 car chaque octet binaire a c'est 2 caracteres hexadecimal 
    ' +1 ' = on ajoute 1 pour '\0'
    */

    int success = 0; //pour touver le hash valide (difficulty = 4)

    printf("Minage du bloc en cours...\n");

    while (!success) { //"tant que on a pas trouver un bon hash"

        //--- créer une chaîne avec les infos du bloc + le Nonce actuel ---
        // sprintf pour mettre l'index, le timestamp, la difficulty et le Nonce dans le buffer
        sprintf(buffer, "%s%ld%s%d", (char*)bloc->previousHash.HashValue, bloc->timestamp, bloc->difficulty, bloc->Nonce);

        //--- calculer le hash de notre buffer
        sha256ofString((BYTE *)buffer, hash_res);

        //--- verifier si le hash commence par "0000" (difficulty = 4)
        // comparerles 4 premiers caracteres de la chaîne hexadécimale
        if (strncmp(hash_res, "0000", 4) == 0) {
            success = 1; //hash trouvé commence par "0000"

            // On enregistre le hash trouve dans le bloc
            //HASH_SIZE -> a changer car HASH_SIZE = 32bytes et hash_res(hash cree) = 64bytes
            strncpy((char*)bloc->actualHash.HashValue, hash_res, HASH_SIZE);

            printf("Bloc miné ! -> Nonce: %d | Hash: %s\n", bloc->Nonce, hash_res);
        } else {

            bloc->Nonce++; // On incremente et on recommence
        }
    }
}

Bloc * create_genesis_block(){

    //--- allocation du memoire pour le bloc ---
    Bloc * genesis = malloc(sizeof(Bloc));
    if(genesis == NULL){
        return NULL;
    }

    //--- initialisation du hash a 0 car c'est le bloc genesis ---
    //memset(adresse, valeur, taille) on force la 'valeur' a l'@ 'adresse' de taille 'taille'
    /* a la place de memset on peut faire la boucle, mais c'est plus longue
    for (int i = 0; i < HASH_SIZE; i++) {
    genesis->previousHash.HashValue[i] = 0;
    }
    */

    memset(genesis->previousHash.HashValue, 0, HASH_SIZE);
    memset(genesis->MerkleRoot.HashValue, 0, HASH_SIZE);

    //--- initialisation des autres parametres ---
    strcpy(genesis->difficulty, "4");   //strcpy car  difficulty est un tab de char
    genesis->Nonce = 0;
    genesis->timestamp = (long)time(NULL); //renvoie le nombre de secondes

    genesis->nbTransactions = 0;
    
    genesis->next = NULL;  //on peut faire qu'il pointe sur soi-meme mais ce n'est pas demander

    //--- on mine le bloc pour remplir actualHash ---
    mine_block(genesis);

    return genesis;
}