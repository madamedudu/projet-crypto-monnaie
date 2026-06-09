#include "sha256_utils.h"
#include "define.h"
#include "transaction.h"
#include "bloc.h" //creation d'un bloc de facon automatique 
#include "utxo.h"
#include "cryptographie.h"
// Merkle Root
// Transforme les données d'une transaction en chaîne puis calcule son SHA256
void hash_transaction(Transaction trans, char hashRes[65]) {
    char buffer[512]; // Buffer pour concaténer les infos de la transaction
    memset(buffer, 0, sizeof(buffer)); // Initialisation du buffer à zéro pour éviter les données résiduelles
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
 
void merkle_root(Slist *liste, int nb_tx, char root[65]) {
    if (nb_tx == 0) {
        strcpy(root, "0000000000000000000000000000000000000000000000000000000000000000");
        return;
    }

    // On stocke les hashs de base (les feuilles)
    char etage_courant[MAXTX][65];
    Slist *temp = liste;
    
    // On parcourt la liste Slist pour remplir le premier étage
    for (int i = 0; i < nb_tx && temp != NULL; i++) {
        Transaction *tx = (Transaction *)temp->info;
        hash_transaction(*tx, etage_courant[i]);
        temp = temp->next;
    }

    // On réduit l'arbre etage par etage
    int nb_actuel = nb_tx;
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

//on fait d'abord une vérification du bloc genesis, ensuite on vérifie que le hash du bloc courant correspond au hash du bloc précédent

int verification_blockchain(Blockchain *bc){
    if (bc == NULL || bc->blocklist == NULL) {
        return 0; // Blockchain invalide
    }

    
    Slist *courant = bc->blocklist; //on est sur le 1er bloc de la blockchain
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
    merkle_root(bloc->transactions, bloc->nbTx, merkle_calcul); //(Zizu) J'ai enleve le forcage de type que t'as fait car j'ai corrige ma fonction en remplacant avec Slist
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
    //vérifie que le hash du bloc commence par le bon nb de 0
    for (int i = 0; i < difficulte; i++) {
        if (bloc->blockHash[i] != '0') {
            return 0;
        }
    }
    return 1; 
}

//on verfifie le hash du bloc  
int verification_hash_bloc(Block *bloc){
    char buffer[512]; 
    memset(buffer, 0, sizeof(buffer));
    char hash_recalcule[HASHLENGTH]; 
    memset(hash_recalcule, 0, sizeof(hash_recalcule));
    //hash généré au minage 
    sprintf(buffer, "%d%s%ld%s%ld", bloc->index,(char*)bloc->previousHash,(long)bloc->timestamp,(char*)bloc->merkleTree,bloc->nonce);
    sha256ofString((BYTE *)buffer, hash_recalcule);
    return strcmp(hash_recalcule, (char*)bloc->blockHash) == 0;
}

// Fonction pour détruire un bloc qui n'a pas pu être ajouté à la chaîne
void liberer_bloc_rejete(Block *b) {
    if (b == NULL) return;
    
    // Libérer toutes les transactions du bloc
    Slist *tx_courante = b->transactions;
    while (tx_courante != NULL) {
        Transaction *tx = (Transaction *)tx_courante->info;

        // 1. Libérer les outputs et le lockingScript (strdup)
        Slist *out_courant = tx->lstOutputs;
        while (out_courant != NULL) {
            TxOutputs *out = (TxOutputs *)out_courant->info;
            if (out != NULL) {
                for (int i = 0; i < LOCK_SCRIPT_SIZE; i++) {
                    if (out->lockingScript[i] != NULL) free(out->lockingScript[i]);
                }
                free(out);
            }
            Slist *tmp = out_courant;
            out_courant = out_courant->next;
            free(tmp);
        }

        // 2. Libérer les noeuds des inputs (eviter fuites valgrind)
        Slist *in_courant = tx->lstInputs;
        while (in_courant != NULL) {
            Slist *tmp = in_courant;
            in_courant = in_courant->next;
            free(tmp);
        }

        // 3. Libérer la transaction
        free(tx);
        Slist *noeud_tx_a_supprimer = tx_courante;
        tx_courante = tx_courante->next;
        free(noeud_tx_a_supprimer);
    }

    // 4. Libérer le bloc lui-même
    free(b);
}
//ajout bloc à la blockchain
int ajouter_bloc_blockchain(Blockchain *bc, Block *nouveau_bloc, int difficulte) {
    if (bc == NULL || nouveau_bloc == NULL) {
        return 0; 
    }
    if (bc->nbBlocks >= MAX_BLOCKS) {
        printf("[Erreur] La blockchain est pleine (Maximum %d blocs atteints).\n", MAX_BLOCKS);
        return 0; 
    }
    // ajouter les blocs suivants
    if (bc->blocklist == NULL) {
        printf("Erreur : La blockchain n'a pas été initialisée (pas de Genesis).\n");
        return 0;
    }
    //verification des tests

    if (!verification_hash_bloc(nouveau_bloc)) {
        printf("Hash du bloc invalide.\n");
        return 0; 
    }
    if (!verification_merkle_bloc(nouveau_bloc))
    {
        printf("Merkle root du bloc invalide.\n");
        return 0;
    }
    if (!verification_preuve_travail(nouveau_bloc, difficulte)) {
        printf("Refusé : La preuve de travail n'est pas respectée.\n");
        return 0;
    }

    //add bloc à la fin de la liste chaînée

        //parcours de la liste pour trouver le dernier élément
    Slist *courant = bc->blocklist;
    while (courant->next != NULL) {
        courant = courant->next;   
    }
    Block *dernier_bloc = (Block *)courant->info; // on recup le dernier bloc

        //vérif que le hash du nouveau= hash du dernier bloc
    if (strcmp((char *)nouveau_bloc->previousHash, (char *)dernier_bloc->blockHash) != 0) {
        printf("Erreur : Previous Hash != Hash du dernier bloc\n");
        return 0;   
    }

        //add nouveau bloc à la fin de la liste
    Slist *nouveau_noeud = (Slist *)malloc(sizeof(Slist));
    if (nouveau_noeud == NULL) {
        printf("Erreur : Impossible d'allouer de la mémoire pour le nouveau bloc.\n");
        return 0;
    }
    nouveau_noeud->info = (void *)nouveau_bloc;
    nouveau_noeud->next = NULL;
    courant->next = nouveau_noeud;

    printf("Bloc %d ajouté à la blockchain !\n", nouveau_bloc->index);
    bc->nbBlocks++;

    return 1; 
}


//liberer la blockchain
void liberer_blockchain(Blockchain *bc) {
    if (bc == NULL) return;
    Slist *bloc_courant = bc->blocklist;
    while (bloc_courant != NULL) {
        Block *b = (Block *)bloc_courant->info;
        
        // Libérer toutes les transactions du bloc
        Slist *tx_courante = b->transactions;
        while (tx_courante != NULL) {
            Transaction *tx = (Transaction *)tx_courante->info;

            //liberer les outputs (TxOutputs + lockingScript faits par strdup) + leurs noeuds
            Slist *out_courant = tx->lstOutputs;
            while (out_courant != NULL) {
                TxOutputs *out = (TxOutputs *)out_courant->info;
                if (out != NULL) {
                    for (int i = 0; i < LOCK_SCRIPT_SIZE; i++) {
                        if (out->lockingScript[i] != NULL) free(out->lockingScript[i]);
                    }
                    free(out);
                }
                Slist *tmp = out_courant;
                out_courant = out_courant->next;
                free(tmp);
            }

            //liberer les noeuds des inputs (les Utxo sont liberes via le registre, on ne libere que les noeuds)
            Slist *in_courant = tx->lstInputs;
            while (in_courant != NULL) {
                Slist *tmp = in_courant;
                in_courant = in_courant->next;
                free(tmp);
            }

            // Libérer la transaction et son noeud
            free(tx);
            Slist *noeud_tx_a_supprimer = tx_courante;
            tx_courante = tx_courante->next;
            free(noeud_tx_a_supprimer);
        }

        // Libérer le bloc et son noeud
        free(b);
        Slist *noeud_bloc_a_supprimer = bloc_courant;
        bloc_courant = bloc_courant->next;
        free(noeud_bloc_a_supprimer);
    }
    
    // Libérer la structure blockchain mère
    free(bc);
}

int utxo_est_depense(Blockchain *bc, char *txid, int outIndex) {
    if (bc == NULL || txid == NULL) {
        return 0; // pas de blockchain ou txid invalide
    }

    // parcourir tous les blocs
    Slist *bloc_courant = bc->blocklist;
    while (bloc_courant != NULL) {
        Block *bloc = (Block *)bloc_courant->info;
        if (bloc != NULL) {
            // parcourir chaque transaction
            Slist *tx_courant = bloc->transactions;
            while (tx_courant != NULL) {
                Transaction *tx = (Transaction *)tx_courant->info;
                if (tx != NULL) {
                    // regarder tous les inputs de la transaction
                    Slist *input_courant = tx->lstInputs;
                    while (input_courant != NULL) {
                        Utxo *input_utxo = (Utxo *)input_courant->info;
                        if (input_utxo != NULL && strcmp((char *)input_utxo->hash, txid) == 0 && input_utxo->indexOutput == outIndex) {
                            return 1; // l'UTXO est dépensé
                        }
                        input_courant = input_courant->next;
                    }
                }
                tx_courant = tx_courant->next;
            }
        }
        bloc_courant = bloc_courant->next;
    }

    return 0; // pas dépensé
}

Slist* reconstruire_wallet(Blockchain *bc, char *address) {
    if (bc == NULL || bc->blocklist == NULL || address == NULL) {
        return NULL; // pas de blockchain ou adresse invalide
    }

    Slist *wallet = NULL;
    Slist *bloc_courant = bc->blocklist;

    // parcourir tous les blocs de la blockchain
    while (bloc_courant != NULL) {
        Block *bloc = (Block *)bloc_courant->info;
        if (bloc != NULL) {
            // parcourir toutes les transactions du bloc
            Slist *tx_courant = bloc->transactions;
            while (tx_courant != NULL) {
                Transaction *tx = (Transaction *)tx_courant->info;
                if (tx != NULL) {
                    Slist *out_courant = tx->lstOutputs;
                    int index = 0;
                    // parcourir chaque sortie
                    while (out_courant != NULL) {
                        TxOutputs *output = (TxOutputs *)out_courant->info;
                        if (output != NULL && output->lockingScript[0] != NULL) {
                            //le lock contient la pubkey, on recalcule l'adresse pour comparer
                            int appartient = 0;
                            if (output->lockingScript[1] != NULL &&
                                strcmp(output->lockingScript[1], "DUP") == 0) {
                                char addr_calc[ADDRESS_LEN];
                                calc_address(addr_calc, output->lockingScript[0]);
                                if (strcmp(addr_calc, address) == 0) appartient = 1;
                            } else {
                                //cas sans pubkey (FEES) on compare direct
                                if (strcmp(output->lockingScript[0], address) == 0) appartient = 1;
                            }

                            if (appartient && !utxo_est_depense(bc, (char *)tx->txid, index)) {
                                Utxo *wallet_utxo = malloc(sizeof(Utxo));
                                if (wallet_utxo != NULL) {
                                    strncpy((char *)wallet_utxo->hash, (char *)tx->txid, HASHLENGTH);
                                    wallet_utxo->hash[HASHLENGTH - 1] = '\0';
                                    wallet_utxo->indexOutput = index;
                                    wallet_utxo->txOut = output;
                                    wallet = inserer_en_queue(wallet, wallet_utxo);
                                }
                            }
                        }
                        index++;
                        out_courant = out_courant->next;
                    }
                }
                tx_courant = tx_courant->next;
            }
        }
        bloc_courant = bloc_courant->next;
    }

    return wallet; // retourne la liste de UTXO trouvés
}
