/*
le define.h va permettre de créer les structures de données utilisées pas tous les fichiers .c du projet,

*/
#ifndef DEFINE_H
#define DEFINE_H

#include <time.h>

// constantes
#define MAX_TRANSACTIONS 10
#define HASH_SIZE 32 // taille du hash en bytes 32 bytes = 256 bits
#define ADRESS_SIZE 50  

//structure de données pour le hash
typedef struct {
    unsigned char HashValue[HASH_SIZE];
} Hash;

//structure de données pour une transaction
typedef struct {
    double amount; //montant de la transaction
    char sender[ADRESS_SIZE]; // Nom de l'envoyeur 
    char receiver[ADRESS_SIZE]; // Nom du destinataire 
    long timestamp; //heure de création de la transaction

    // unsigned char public_key[65]; //taille d'une clé public en bytes
    // unsigned char signature[64]; //taille d'une signature en bytes
} Transaction;


//structure de données pour un bloc
typedef struct {
    Hash previousHash; //hash du bloc précédent
    Hash MerkleRoot; //racine de l'arbre de Merkle des transactions du bloc
    char difficulty[4]; //difficulté du minage du bloc
    int Nonce; //nombre utilisé pour trouver un hash valide (preuve de travail
    long timestamp; //heure de création du bloc

    //partie des transactions du bloc
    int nbTransactions; //nombre de transactions dans le bloc
    Transaction transactions[MAX_TRANSACTIONS]; //tableau de transactions du bloc "on en a dix maximum par bloc"

    //partie hashage du bloc
    Hash actualHash; //hash actuel du bloc

    //liste chaînée des blocs
    struct Bloc* next; //pointeur vers le bloc suivant
}Bloc;

typedef struct{
    Bloc* head; //pointeur vers le premier bloc de la blockchain (genesis block)
    int length; //nombre de blocs dans la blockchain
} Blockchain;


#endif