#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "define.h"
#include "sha256_utils.h"



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