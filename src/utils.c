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
void export_blockchain_json(currency_t *currency, const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printf("erreur fopen \n");
        return;
    }

    Blockchain *bc = currency->bc;

    fprintf(file, "{\n");
    fprintf(file, "  \"Name\": \"%s\",\n", currency->currency_name);
    fprintf(file, "  \"Money supply\": %ld,\n", currency->moneySupply);
    fprintf(file, "  \"blockchain\": {\n");
    fprintf(file, "    \"Difficulty\": %d,\n", bc->difficulty);
    fprintf(file, "    \"Nb blocks\": %d,\n", bc->nbBlocks);
    fprintf(file, "    \"Actual reward\": %d,\n", bc->reward4mining);
    fprintf(file, "    \"blocks\": [\n");

    Slist *BlocCourant = bc->blocklist;

    while (BlocCourant != NULL) {
        Block *block = (Block *)BlocCourant->info;

        char *time_str = ctime(&block->timestamp);
        time_str[strcspn(time_str, "\n")] = '\0';

        fprintf(file, "      {\n");
        fprintf(file, "        \"index\": %d,\n", block->index);
        fprintf(file, "        \"time stamp\": \"%s\",\n", time_str);
        fprintf(file, "        \"previous hash\": \"%s\",\n", block->previousHash);
        fprintf(file, "        \"Nb tx\": %d,\n", block->nbTx);
        fprintf(file, "        \"transactions\": [\n");

        Slist *TransactionCourante = block->transactions;
        while (TransactionCourante != NULL) {
            Transaction *tx = (Transaction *)TransactionCourante->info;

            char *tx_time = ctime(&tx->timestamp);
            tx_time[strcspn(tx_time, "\n")] = '\0';

            fprintf(file, "          {\n");
            fprintf(file, "            \"TxId\": \"%s\",\n", tx->txid);
            fprintf(file, "            \"Timestamp\": \"%s\",\n", tx_time);
            fprintf(file, "            \"Sender\": \"%s\",\n", tx->adSender);
            fprintf(file, "            \"Receiver\": \"%s\",\n", tx->adReceiver);
            fprintf(file, "            \"Amount\": %ld,\n", tx->txAmount);
            fprintf(file, "            \"Nb inputs\": %d,\n", tx->nbInputs);
            fprintf(file, "            \"Nb outputs\": %d,\n", tx->nbOutputs);
            fprintf(file, "            \"Comments\": \"%s\"\n", tx->comment);

            if (TransactionCourante->next == NULL) fprintf(file, "          }\n");
            else fprintf(file, "          },\n");

            TransactionCourante = TransactionCourante->next;
        }

        fprintf(file, "        ],\n");
        fprintf(file, "        \"Merkle root\": \"%s\",\n", block->merkleTree);
        fprintf(file, "        \"current hash\": \"%s\",\n", block->blockHash);
        fprintf(file, "        \"nonce\": %ld,\n", block->nonce);
        fprintf(file, "        \"miner name\": \"%s\",\n", block->minerName);
        fprintf(file, "        \"comment\": \"%s\"\n", block->comment);

        if (BlocCourant->next == NULL) fprintf(file, "      }\n");
        else fprintf(file, "      },\n");

        BlocCourant = BlocCourant->next;
    }

    fprintf(file, "    ]\n");
    fprintf(file, "  }\n");
    fprintf(file, "}\n");

    fclose(file);

    printf("L'export json est terminé, le fichier est : %s\n", filename);
}