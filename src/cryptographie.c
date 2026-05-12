#include "cryptographie.h"
#include "sha256_utils.h"
#include <stdlib.h>
#include "define.h"






void generer_cles (Account * compte ){
    if(compte == NULL) return;

    char buffer[64];
    sprintf(buffer, "%ld%d", time(NULL), rand());

    //générer la clé privé
    sha256ofString((BYTE*)buffer, (char*)compte->priv_key);

    //générer la clé publique
    sha256ofString(compte->priv_key, (char*)compte->pub_key); //c'est le hash de la clé privé

    //générer l'adresse
    sha256ofString(compte->pub_key, (char*)compte->address); //c'est le hash de la clé publique
}






void signer_transaction(Transaction *transaction, BYTE *cle_privee, BYTE *signature){
    if(transaction == NULL || cle_privee == NULL || signature == NULL) return;

    char buffer_donnees_transaction[512]; //contient toute les données de la transaction

    //on transforme la transaction en chaîne parce que SHA256 prend un chaîne
    sprintf(buffer_donnees_transaction, "%s%s%ld%ld",transaction->adSender, transaction->adReceiver, transaction->txAmount, transaction->timestamp);

    //ici on simule une signature
    char buffer_signature[600]; //va contenit la signature
    sprintf(buffer_signature, "%s%s", buffer_donnees_transaction, cle_privee); //dans ke buffer_signature on met transaction + clé privée
    
    //on calcule maintenant la signature
    sha256ofString((BYTE *)buffer_signature, (char *)signature);

    printf("Transaction signée : %s\n", signature);
}






int verifier_signature(Transaction *transaction, BYTE *cle_privee, BYTE *signature){
    if(transaction == NULL || cle_privee == NULL || signature == NULL) return 0;

    char buffer[512];
    char buffer_signature[600];
    char signature_calculee[65];

    //Recréer les données de la transaction
    sprintf(buffer, "%s%s%ld%ld", transaction->adSender, transaction->adReceiver, transaction->txAmount, transaction->timestamp);

    //on ajouter la clé privée
    sprintf(buffer_signature, "%s%s", buffer, cle_privee);

    //on recalcule la signature
    sha256ofString((BYTE *)buffer_signature, signature_calculee);

    //on compare la signature calculée avec la signature qu'on nous a fournie
    if (strcmp(signature_calculee, (char*)signature) == 0){
        return 1; //signature valide
    } else {
        return 0; //signature invalide
    }
}





void creer_lock_script(TxOutputs *out, BYTE *cle_publique){
    if(out == NULL || cle_publique == NULL) return;

    //on creer et alloue une chaîne pour stocker le script
    out->lockingScript[0] = malloc(65);
    if(out->lockingScript[0] == NULL){
        perror("erreur allocation lock script");
    }

    sha256ofString(cle_publique, out->lockingScript[0]); //on calcule le hash de la clé publique et on le stocke dans UTXO

    printf("Lock script créé : %s\n", out->lockingScript[0]);
}





void creer_unlock_script(TxInputs *input, BYTE *signature, BYTE *cle_publique){
    if (input == NULL || signature == NULL || cle_publique == NULL) return;

    //on alloue la place pour la signature
    input->unlockingScript[0] = malloc(HASHLENGTH);
    if (input->unlockingScript[0] == NULL) {
        perror("erreur allocation signature");
    }

    //Allocation mémoire pour la clé publique
    input->unlockingScript[1] = malloc(HASHLENGTH);
    if (input->unlockingScript[1] == NULL) {
        perror("erreur allocation clé publique");
    }

    
    strcpy(input->unlockingScript[0], (char*)signature); //Copie de la signature
    strcpy(input->unlockingScript[1], (char*)cle_publique); //Copie de la clé publique

    printf("Unlock script créé\n");
}





int executer_script(TxInputs* input, TxOutputs* output, Transaction* transaction){
    if(input == NULL || output == NULL || transaction == NULL) return 0;

    //on vérifie que le hash = lockscript
    if (strcmp(input->unlockingScript[1], output->lockingScript[0]) != 0) {
        printf("Erreur : mauvais proprietaire\n");
        return 0;
    }

    //Tout est valide alors on rnevoie 1
    return 1;
}