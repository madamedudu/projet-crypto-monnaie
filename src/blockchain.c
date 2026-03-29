#include "sha256_utils.h"
#include "define.h"

// Transforme les données d'une transaction en chaîne puis calcule son SHA256
void hash_transaction(Transaction trans, char hashRes[65]) {
    char buffer[512]; // Buffer pour concaténer les infos de la transaction
    
    // On crée une chaine pour representer la transaction
    // "user1user2500001711032000"
    sprintf(buffer, "%s%s%ld%ld", trans.adSender, trans.adReceiver, trans.txAmount, trans.timestamp);
    
    // On calcule le hash de cette chaîne
    sha256ofString((BYTE *)buffer, hashRes);
}

// Concatène deux hashs et calcule le hash du parent.

void hash_parent(char left[65], char right[65], char parentRes[65]) {
    char concat[129]; // 64 + 64 + \0
    sprintf(concat, "%s%s", left, right);
    sha256ofString((BYTE *)concat, parentRes);
}

//La fonction principale qui calcule la Merkle Root d'un tableau de transactions
 
void merkle_root(Transaction transactions[], int nb_transactions, char root[65]) {
    if (nb_transactions == 0) {
        strcpy(root, "0000000000000000000000000000000000000000000000000000000000000000");
        return;
    }

    // On stocke les hashs de base (les feuilles)
    char etage_courant[MAXTX][65];
    for (int i = 0; i < nb_transactions; i++) {
        hash_transaction(transactions[i], etage_courant[i]);
    }

    // On réduit l'arbre etage par etage
    int nb_actuel = nb_transactions;
    while (nb_actuel > 1) {
        int nb_suivant = 0;
        for (int i = 0; i < nb_actuel; i += 2) {
            if (i + 1 < nb_actuel) {
                // On a une paire : on calcule le parent normalement
                hash_parent(etage_courant[i], etage_courant[i+1], etage_courant[nb_suivant]);
            } else {
                // Nombre impair : on double le dernier hash
                hash_parent(etage_courant[i], etage_courant[i], etage_courant[nb_suivant]);
            }
            nb_suivant++;
        }
        nb_actuel = nb_suivant;
    }

    // Le résultat final est dans le premier emplacement
    strcpy(root, etage_courant[0]);
}
// strcpy(nouveau_bloc->MerkleRoot.HashValue, racine_calculee);

//---- bloc de chainage ----

//on fait d'abord une vérification du bloc genesis, ensuite on vérifie que le hash du bloc courant correspond au hash du bloc précédent
int verification_blockchain(Blockchain *bc){
    if (bc == NULL || bc->blocklist == NULL) {
        return 0; // Blockchain invalide
    }

    // Vérification du bloc genesis
    Slist *courant = bc->blocklist; // on est sur le 1er bloc de la blockchain (genesis)
    Block *bloc_precedent = NULL;

    int i= 0;
    while (courant != NULL) {
        Block *courant_bloc = (Block *)courant->info; // on recupere le bloc courant (le bloc courant c'est l'info de la liste chainee)
        //cas un on est sur le bloc genesis
        if (i == 0) {
            // Vérification du bloc genesis
            if (strcmp(courant_bloc->previousHash, "0000000000000000000000000000000000000000000000000000000000000000") != 0) {
                return 0; // Bloc genesis invalide
            }
        //cas deux on est sur les autres blocs
        } else {
            // Vérification du hash du bloc courant correspond au hash du bloc précédent
            if (strcmp(courant_bloc->previousHash, bloc_precedent->blockHash) != 0) {
                return 0; // Bloc invalide  
            }
        }
        bloc_precedent = courant_bloc;
        courant = courant->next; // on passe au bloc suivant
        i++;
    }
    return 1; // Blockchain valide 
}

//deuxieme verification, on recalcul le hash du bloc avec le merkle root (on va le recalculer sur chaque bloc de la blockchain ensuite)


