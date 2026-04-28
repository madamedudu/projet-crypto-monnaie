#include <stdio.h>     
#include <stdlib.h>    
#include "define.h"    
#include "utils.h"     

//afficher tout les comptes avec leur soldes dans la console
//----------MODIFICATION CHIRINE------------
void afficher_accounts(ListeAccounts liste) {
    printf("----- LISTE DES COMPTES :-----\n");
    for (int i = 0; i < liste.nb_accounts; i++) {
        printf("%s : %ld BT\n", liste.accounts[i].str, liste.accounts[i].balance);
    }
    printf("----------------------------------\n");
}



//fonction qui retranscrit la blockchain dans un fichier .json 
void export_blockchain_json(ListeAccounts liste, Blockchain *blockchain, const char* filename){
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf("Erreur ouverture fichier\n");
        return;
    }
    
    fprintf(file, "{\n");

    //--------------------- NAME + MONEY ---------------------
    fprintf(file, "\"Name\": \"bit thune\",\n");

    long money = 0;
    for(int i = 0; i < liste.nb_accounts; i++){
        money += liste.accounts[i].balance;
    }
    fprintf(file, "\"Money supply\": %d,\n", blockchain->nbBlocks * HELIREWARD);

    //---------------------- BLOCKCHAIN -------------------------
    fprintf(file, "\"blockchain\": {\n");
    fprintf(file, "\"Difficulty\": %d,\n", blockchain->difficulty);
    fprintf(file, "\"Nb blocks\": %d,\n", blockchain->nbBlocks);
    fprintf(file, "\"Actual reward\": %d,\n", blockchain->reward4mining);

    fprintf(file, "\"blocks\":[\n");

    if(blockchain != NULL && blockchain->blocklist != NULL) {
        Slist *BlocCourant = blockchain->blocklist;

        while (BlocCourant != NULL) {
            Block *block = (Block*) BlocCourant->info;

            // timestamp lisible
            char time_str[100];
            strftime(time_str, sizeof(time_str), "%a %b %d %H:%M:%S %Y", localtime(&block->timestamp));

            fprintf(file, "  {\n");
            fprintf(file, "  \"index\": %d,\n", block->index);
            fprintf(file, "  \"time stamp\": \"%s\",\n", time_str);
            fprintf(file, "  \"previous hash\": \"%s\",\n", block->previousHash);
            fprintf(file, "  \"Nb tx\": %d,\n", block->nbTx);

            //---------------- TRANSACTIONS ----------------
            fprintf(file, "  \"transactions\":[\n");

            Slist *TransactionCourante = block->transactions;

            while (TransactionCourante != NULL) {
                Transaction *tx = (Transaction*) TransactionCourante->info;

                char tx_time[100];
                strftime(tx_time, sizeof(tx_time), "%a %b %d %H:%M:%S %Y", localtime(&tx->timestamp));

                fprintf(file, "  {\n");

                fprintf(file, "    \"TxId\": \"%s\",\n", tx->txid);
                fprintf(file, "    \"Timestamp\": \"%s\",\n", tx_time);
                fprintf(file, "    \"Sender\": \"%s\",\n", tx->adSender);
                fprintf(file, "    \"Receiver\": \"%s\",\n", tx->adReceiver);
                fprintf(file, "    \"Amount\": %ld,\n", tx->txAmount);

                //---------------- INPUTS ----------------
                fprintf(file, "    \"Nb inputs\": %d,\n", tx->nbInputs);
                fprintf(file, "    \"Inputs list\":[\n");

                Slist *in = tx->lstInputs;
                while (in != NULL) {
                    TxInputs *input = (TxInputs*) in->info;

                    fprintf(file, "      {\n");
                    fprintf(file, "      \"txid\": \"%s\",\n", input->txHash);
                    fprintf(file, "      \"index\": %d\n", input->indexOutput);

                    if (in->next == NULL)
                        fprintf(file, "      }\n");
                    else
                        fprintf(file, "      },\n");

                    in = in->next;
                }
                fprintf(file, "    ],\n");

                //---------------- OUTPUTS ----------------
                fprintf(file, "    \"Nb outputs\": %d,\n", tx->nbOutputs);
                fprintf(file, "    \"Outputs list\":[\n");

                Slist *out = tx->lstOutputs;
                int index = 0;

                while (out != NULL) {
                    TxOutputs *output = (TxOutputs*) out->info;

                    fprintf(file, "      {\n");
                    fprintf(file, "      \"out index\": %d,\n", index);
                    fprintf(file, "      \"Catégorie\": \"transaction\",\n");
                    fprintf(file, "      \"Timestamp\": \"%s\",\n", tx_time);
                    fprintf(file, "      \"lock\": \"%s\",\n", output->lockingScript[0]);
                    fprintf(file, "      \"Amount\": %ld\n", output->amount);

                    if (out->next == NULL)
                        fprintf(file, "      }\n");
                    else
                        fprintf(file, "      },\n");

                    out = out->next;
                    index++;
                }
                fprintf(file, "    ],\n");

                //---------------- COMMENT ----------------
                fprintf(file, "    \"Comments\": \"%s\"\n", tx->comment);

                if(TransactionCourante->next == NULL)
                    fprintf(file, "  }\n");
                else
                    fprintf(file, "  },\n");

                TransactionCourante = TransactionCourante->next;
            }

            fprintf(file, "  ],\n");

            //---------------- BLOCK INFOS ----------------
            fprintf(file, "\"Merkle root\": \"%s\",\n", block->merkleTree);
            fprintf(file, "\"current hash\": \"%s\",\n", block->blockHash);
            fprintf(file, "\"nonce\": %ld,\n", block->nonce);
            fprintf(file, "\"miner name\": \"%s\",\n", block->minerName);
            fprintf(file, "\"comment\": \"%s\"\n", block->comment);

            if(BlocCourant->next == NULL)
                fprintf(file, "}\n");
            else
                fprintf(file, "},\n");

            BlocCourant = BlocCourant->next;
        }
    }

    fprintf(file, "]\n"); // fin blocks
    fprintf(file, "}\n"); // fin blockchain
    fprintf(file, "}\n"); // fin JSON global

    fclose(file);
    printf("FIN EXPORT JSON : %s\n", filename);
}