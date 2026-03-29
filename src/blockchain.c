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
    Slist *courant = bc->blocklist; //on est sur le 1er bloc de la blockchain (genesis)
    Block *bloc_precedent = NULL;

    int i= 0;
    while (courant != NULL) {
        Block *courant_bloc = (Block *)courant->info; //on recupere le bloc courant 
        //cas un: on est sur le bloc genesis
        if (i == 0) {
            // Vérification du bloc genesis (chirine : j'ai ajouté un char pour que ça passe les warnings)
            if (strcmp((char*)courant_bloc->previousHash, "0000000000000000000000000000000000000000000000000000000000000000") != 0) {
                return 0; //Bloc genesis invalide
            }
        //cas deux: on est sur les autres blocs
        } else {
            //Vérification du hash du bloc courant correspond au hash du bloc précédent
            if (strcmp((char *)courant_bloc->previousHash, (char *)bloc_precedent->blockHash) != 0) {
                return 0; //Bloc courant invalide  
            }
        }
        bloc_precedent = courant_bloc;
        courant = courant->next; // on passe au bloc suivant
        i++;
    }
    return 1; // Blockchain valide 
}

//on verifie le merkle root du bloc
int verification_merkle_bloc(Block *bloc){
    char merkle_calcul[65];
    merkle_root((Transaction *)bloc->transactions, bloc->nbTx, merkle_calcul);
    return strcmp(merkle_calcul, (char *)bloc->merkleTree) == 0;
}

//on vérifie le merkle root de la blockchain
int verification_merkle_blockchain(Blockchain *bc){
    // Blockchain inexistante ou vide
    if (bc == NULL || bc->blocklist == NULL) {
        return 0; 
    }
    Slist *courant = bc->blocklist; // on est sur le 1er bloc
    while (courant != NULL) {
        Block *courant_bloc = (Block *)courant->info; // on recupere le bloc courant
        if (verification_merkle_bloc(courant_bloc)==0) {
            return 0; // Blockchain invalide si un bloc a une merkle root incorrecte
        }
        courant = courant->next; 
    }
    return 1; 

}

//on verifie la preuve de travail du bloc
int verification_preuve_travail(Block *bloc,int difficulte){
    // On vérifie que le hash du bloc commence par le nombre de zéros requis
    for (int i = 0; i < difficulte; i++) {
        if (bloc->blockHash[i] != '0') {
            return 0; // Preuve de travail invalide
        }
    }
    return 1; // Preuve de travail valide
}

//on verfifie le hash du bloc  
int verification_hash_bloc(Block *bloc){
    char buffer[512]; // Buffer pour concaténer les infos de la transaction
    char hash_recalcule[HASHLENGTH]; // Buffer pour stocker le hash calculé
    //hash généré au minage 
    sprintf(buffer, "%d%s%ld%s%ld", bloc->index,(char*)bloc->previousHash,(long)bloc->timestamp,(char*)bloc->merkleTree,bloc->nonce);
    sha256ofString((BYTE *)buffer, hash_recalcule);
    return strcmp(hash_recalcule, (char*)bloc->blockHash) == 0;
}

//ajout bloc à la blockchain
int ajouter_bloc_blockchain(Blockchain *bc, Block *nouveau_bloc, int difficulte) {
    if (bc == NULL || nouveau_bloc == NULL) {
        return 0; // Échec de l'ajout
    }

    // Cette fonction ne sert donc qu'à ajouter les blocs suivants.
    if (bc->blocklist == NULL) {
        printf("Erreur : La blockchain n'a pas été initialisée (pas de Genesis).\n");
        return 0;
    }
    //verification des tests

    if (!verification_hash_bloc(nouveau_bloc)) {
        printf("Hash du bloc invalide.\n");
        return 0; // Échec de l'ajout
    }
    if (!verification_merkle_bloc(nouveau_bloc))
    {
        printf("Merkle root du bloc invalide.\n");
        return 0; // Échec de l'ajout
    }
    if (!verification_preuve_travail(nouveau_bloc, difficulte)) {
        printf("Refusé : La preuve de travail n'est pas respectée.\n");
        return 0;
    }

    // Ajout du bloc à la fin de la liste chaînée

        //1-on fait le parcours de la liste pour trouver le dernier élément
    Slist *courant = bc->blocklist;
    while (courant->next != NULL) {
        courant = courant->next;   
    }
    Block *dernier_bloc = (Block *)courant->info; // on recupere le dernier bloc

        //2-on vérifie que le hash du nouveau bloc correspond au hash du dernier bloc
    if (strcmp((char *)nouveau_bloc->previousHash, (char *)dernier_bloc->blockHash) != 0) {
        printf("Erreur : Previous Hash != Hash du dernier bloc\n");
        return 0; // Échec de l'ajout   
    }

        //3-on ajoute le nouveau bloc à la fin de la liste (chainage)
    Slist *nouveau_noeud = (Slist *)malloc(sizeof(Slist));
    if (nouveau_noeud == NULL) {
        printf("Erreur : Impossible d'allouer de la mémoire pour le nouveau bloc.\n");
        return 0;
    }
    nouveau_noeud->info = (void *)nouveau_bloc;
    nouveau_noeud->next = NULL;
    courant->next = nouveau_noeud;

    printf("Bloc %d ajouté à la blockchain !\n", nouveau_bloc->index);
    bc->nbBlocks++; //mise à jour du nombre de blocs dans la blockchain

    return 1; // Succès de l'ajout
}