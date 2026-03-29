#include <stdio.h>     
#include <stdlib.h>    
#include "define.h"    
#include "utils.h"     

//afficher tout les comptes avec leur soldes dans la console
void afficher_users(ListeUsers liste) {
    printf("----- LISTE DES UTILISATEURS :-----\n");

    for (int i = 0; i < liste.nb_users; i++) {
        printf("%s : %.2f BT\n", liste.users[i].adresse, liste.users[i].solde);
    }

    printf("----------------------------------\n");
}

//fonction qui retranscrit la blockchain dans un fichier .json 
//on doit afficher : ensemble des blocs, les transactions et les utilisateurs 
/*

Utilisateurs
Blockchain
 → Bloc 1
     → transactions
 → Bloc 2
     → transactions
     
*/
void export_blockchain_json(ListeUsers liste, Blockchain *blockchain, const char* filename){
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf("Erreur ouverture fichier\n");
        return;
    }
    fprintf(file, "{\n");







    //--------------------- USERS ---------------------
    fprintf(file, "  \"users\": [\n");
    for(int i = 0; i < liste.nb_users; i++) {
        fprintf(file, "    {\n");
        fprintf(file, "      \"adresse\": \"%s\",\n", liste.users[i].adresse);
        fprintf(file, "      \"solde\": %.2f\n", liste.users[i].solde);
        if (i == liste.nb_users - 1)
            fprintf(file, "    }\n");
        else
            fprintf(file, "    },\n");
    }
    fprintf(file, "  ],\n");







    //---------------------- BLOCKCHAIN -------------------------
    fprintf(file, "  \"blockchain\": [\n");
    if(blockchain != NULL && blockchain->blocklist != NULL) {
        Slist *BlocCourant = blockchain->blocklist;

        //Tant qu'on arrive pas a la fin de la blockchain
        while (BlocCourant != NULL) {
            //informations d'un bloc
            Block *block = (Block*) BlocCourant->info;
            fprintf(file, "    {\n");
            fprintf(file, "      \"index\": %d,\n", block->index);
            fprintf(file, "      \"timestamp\": %ld,\n", block->timestamp);
            fprintf(file, "      \"nonce\": %ld,\n", block->nonce);
            fprintf(file, "      \"miner\": \"%s\",\n", block->minerName);
            fprintf(file, "      \"previousHash\": \"%s\",\n", block->previousHash);
            fprintf(file, "      \"hash\": \"%s\",\n", block->blockHash);





            //-------------- TRANSACTIONS --------------------
            fprintf(file, "      \"transactions\": [\n");
            if (block->transactions != NULL){
                //information d'une transaction qui est dans un blo
                Slist *TransactionCourante = block->transactions;
                while (TransactionCourante != NULL) {
                    Transaction *tx = (Transaction*) TransactionCourante->info;
                    fprintf(file, "        {\n");
                    fprintf(file, "          \"from\": \"%s\",\n", tx->adSender);
                    fprintf(file, "          \"to\": \"%s\",\n", tx->adReceiver);
                    fprintf(file, "          \"amount\": %ld,\n", tx->txAmount);
                    fprintf(file, "          \"timestamp\": %ld\n", tx->timestamp);

                    if(TransactionCourante->next == NULL){
                        fprintf(file, "        }\n");
                    }
                    else{
                        fprintf(file, "        },\n");
                    }
                        
                    TransactionCourante = TransactionCourante->next;
                }
            }

            fprintf(file, "      ]\n");
            if(BlocCourant->next == NULL){
                fprintf(file, "    }\n");
            } 
            else{
                fprintf(file, "    },\n");
            }
            BlocCourant = BlocCourant->next;
        }
    }
    fprintf(file, "  ]\n");
    fprintf(file, "}\n");

    fclose(file);
    printf("FIN EXPORT JSON : %s\n", filename);
}