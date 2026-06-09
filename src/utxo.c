#include <stdio.h>
#include <stdlib.h> 
#include "define.h"
#include "utxo.h"
#include "transaction.h"
#include "script.h"


//voici la liste global
struct Slist *global_utxo_list = NULL;

//-------------CREATION D'UN OUTPUT-----------------------
TxOutputs* creer_output(long montant, char *nom_destinataire, BYTE *pub_key) {
    if (nom_destinataire == NULL) return NULL;
 
    //allocation mémoire du nouveau billet
    TxOutputs *nouveau_output = malloc(sizeof(TxOutputs));
    if (nouveau_output == NULL) {
        printf("[Erreur] Allocation memoire echouee pour l'output.\n");
        return NULL;
    }
    memset(nouveau_output, 0, sizeof(TxOutputs));
 
    nouveau_output->amount = montant;
    nouveau_output->outIndex = 0;
    nouveau_output->timestamp = (long)time(NULL);
    for(int i = 0; i < LOCK_SCRIPT_SIZE; i++){
        nouveau_output->lockingScript[i] = NULL;
    }
 
    if (pub_key != NULL) {
        //lock script : <pubKey> DUP HASH (le DUP HASH donne H(pubkey) au sommet)
        nouveau_output->lockingScript[0] = strdup((char*)pub_key);
        nouveau_output->lockingScript[1] = strdup("DUP");
        nouveau_output->lockingScript[2] = strdup("HASH");
    } else {
        //pas de cle (ex FEES) on met juste le nom
        nouveau_output->lockingScript[0] = strdup(nom_destinataire);
    }

    if (nouveau_output->lockingScript[0] == NULL) {
        printf("[Erreur] strdup echoue pour lockingScript.\n");
        free(nouveau_output);
        return NULL;
    }
 
    return nouveau_output;
}

//ajout d'un UTXO dans la liste globale -> monnaie tracée dès qu'elle rentre
void ajouter_utxo(TxOutputs *output, char *txid_source, int index) {
    if (output == NULL || txid_source == NULL) return;
 
    //creer la structure d'un utxo (l'étiquette qui pointe vers le billet)
    Utxo *nouvel_utxo = malloc(sizeof(Utxo));
    if (nouvel_utxo == NULL) {
        printf("[Erreur] Echec de l'allocation pour le nouvel UTXO.\n");
        return;
    }
 
    //remplissage des champs
    strncpy((char*)nouvel_utxo->hash, txid_source, HASHLENGTH);
    nouvel_utxo->hash[HASHLENGTH - 1] = '\0'; //sécurité au cas où txid_source ne serait pas null-terminé
    nouvel_utxo->indexOutput = index;
    nouvel_utxo->txOut = output;
 
    //ajout à la liste chainée globale avec vérif
    struct Slist *nouveau_noeud = malloc(sizeof(struct Slist));
    if (nouveau_noeud == NULL) {
        printf("[Erreur] Echec allocation du noeud.\n");
        free(nouvel_utxo);
        return;
    }
 
    //chainage en tête de liste
    nouveau_noeud->info = nouvel_utxo;
    nouveau_noeud->next = global_utxo_list;
    global_utxo_list = nouveau_noeud;
}
 
//ici on va rechercher un UTXO par son txid et son index
Utxo* rechercher_utxo(char *txid_source, int index) {
    struct Slist *courant = global_utxo_list;
 
    while (courant != NULL) {
        Utxo *u = (Utxo *)courant->info;
        if (u != NULL && u->indexOutput == index && strcmp((char*)u->hash, txid_source) == 0) {
            return u; // Trouvé !
        }
        courant = courant->next;
    }
    return NULL; // Non trouvé
}
 
//on supprime un UTXO de la liste globale
//à appeler APRES avoir validé la transaction qui le consomme
//IMPORTANT : on ne libère PAS le TxOutputs car il reste dans la blockchain (historique immuable)
void supprimer_utxo(char *txid_source, int index) {
    struct Slist *courant = global_utxo_list;
    struct Slist *precedent = NULL;
 
    while (courant != NULL) {
        Utxo *u = (Utxo *)courant->info;
 
        if (u != NULL && u->indexOutput == index && strcmp((char*)u->hash, txid_source) == 0) {
            //déchaînage
            if (precedent == NULL) {
                global_utxo_list = courant->next;
            } else {
                precedent->next = courant->next;
            }

            free(courant);
            return;
        }
        precedent = courant;
        courant = courant->next;
    }
    //si on arrive ici c'est que l'UTXO n'existe pas, rien à faire
}
 
//-------------AFFICHAGE-----------------------
//affiche tous les UTXO non dépensés (appelée par le main option 7)
void afficher_utxo_global() {
    struct Slist *courant = global_utxo_list;
    int i = 0;
 
    printf("\n=== REGISTRE UTXO GLOBAL ===\n");
 
    while (courant != NULL) {
        Utxo *u = (Utxo *)courant->info;
 
        if (u != NULL && u->txOut != NULL) {
            char *proprietaire = (u->txOut->lockingScript[0] != NULL)
                                  ? u->txOut->lockingScript[0]
                                  : "inconnu";
            printf("[%d] Proprio : %-15s | Montant : %6ld BT | TXID : %.16s...\n",
                   i, proprietaire, u->txOut->amount, (char*)u->hash);
            i++;
        }
        courant = courant->next;
    }
 
    if (i == 0) printf("  Registre vide.\n");
 
    printf("============================\n");
    printf("Total UTXO non depenses : %d\n\n", i);
}
//somme de tous les utxo non depenses = vraie masse monetaire en circulation
long calculer_masse_monetaire() {
    long total = 0;
    struct Slist *courant = global_utxo_list;
    while (courant != NULL) {
        Utxo *u = (Utxo *)courant->info;
        if (u != NULL && u->txOut != NULL) {
            total += u->txOut->amount;
        }
        courant = courant->next;
    }
    return total;
}
void liberer_registre_utxo() {
    struct Slist *courant = global_utxo_list;
    while (courant != NULL) {
        struct Slist *suivant = courant->next;
        Utxo *u = (Utxo *)courant->info;
        if (u != NULL) free(u); //on libere l'etiquette, pas u->txOut
        free(courant);
        courant = suivant;
    }
    global_utxo_list = NULL;
}
// L'algorithme glouton - on a changé le nom de l'emetteur en Account pour pouvoir utiliser sa clé publique dans le script
Slist* select_utxos_greedy(Account *emetteur, long montant_cible, long *somme_recuperee) {
    if (global_utxo_list == NULL || montant_cible <= 0) return NULL;

    Utxo *best_greater = NULL;
    long min_greater_val = -1;
    Slist *selection = NULL;
    *somme_recuperee = 0;

    char *unlock_tmp[UNLOCK_SCRIPT_SIZE];
    char hash_pubkey[65];
    
    // on hache la cle publique de l'emetteur
    sha256ofString((BYTE *)emetteur->pub_key, hash_pubkey);
    
    // unlock : <H(pubKey)> EQ (pas de VER ici, juste pour savoir a qui appartient l'utxo)
    unlock_tmp[0] = hash_pubkey;
    unlock_tmp[1] = "EQ";
    unlock_tmp[2] = NULL;

    // Chercher le "Smallest Greater" (un seul billet qui suffit)
    struct Slist *courant = global_utxo_list;
    while (courant != NULL) {
        Utxo *u = (Utxo *)courant->info;
        if (execute_script(u->txOut->lockingScript, LOCK_SCRIPT_SIZE, unlock_tmp, UNLOCK_SCRIPT_SIZE, NULL) == 1) {
            if (u->txOut->amount >= montant_cible) {
                if (min_greater_val == -1 || u->txOut->amount < min_greater_val) {
                    min_greater_val = u->txOut->amount;
                    best_greater = u;
                }
            }
        }
        courant = courant->next;
    }

    if (best_greater) {
        *somme_recuperee = best_greater->txOut->amount;
        return inserer_en_tete(NULL, best_greater);
    }

    // Accumulation (si aucun billet seul ne suffit)
    courant = global_utxo_list;
    while (courant != NULL && *somme_recuperee < montant_cible) {
        Utxo *u = (Utxo *)courant->info;
        if (execute_script(u->txOut->lockingScript, LOCK_SCRIPT_SIZE, unlock_tmp, UNLOCK_SCRIPT_SIZE, NULL) == 1) {
            selection = inserer_en_tete(selection, u);
            *somme_recuperee += u->txOut->amount;
        }
        courant = courant->next;
    }

    // Si on n'a pas assez de fonds, on nettoie la liste temporaire
    if (*somme_recuperee < montant_cible) {
        while (selection != NULL) {
            Slist *tmp = selection;
            selection = selection->next;
            free(tmp);
        }
        return NULL;
    }

    return selection;
}
//caclul solde des billets
long calculer_solde_reel(struct account *acc) {
    if (acc == NULL) return 0;
    long total = 0;

    char *unlock_tmp[UNLOCK_SCRIPT_SIZE];
    char hash_pubkey[65];
    sha256ofString((BYTE *)acc->pub_key, hash_pubkey);
    unlock_tmp[0] = hash_pubkey;
    unlock_tmp[1] = "EQ";
    unlock_tmp[2] = NULL;

    struct Slist *curr = global_utxo_list;
    while (curr != NULL) {
        Utxo *u = (Utxo *)curr->info;
        if (u && u->txOut && u->txOut->lockingScript[0]) {
            if (execute_script(u->txOut->lockingScript, LOCK_SCRIPT_SIZE,
                               unlock_tmp, UNLOCK_SCRIPT_SIZE, NULL) == 1) {
                total += u->txOut->amount;
            }
        }
        curr = curr->next;
    }
    acc->balance = total;
    return total;
}

//-------------NETTOYAGE-----------------------
//vide toute la liste(fin de marché)
//ici on libère étiquettes + TxOutputs + lockingScript + noeuds
void vider_liste_utxo() {
    struct Slist *courant = global_utxo_list;
    struct Slist *suivant;
 
    while (courant != NULL) {
        suivant = courant->next;
 
        Utxo *utxo_a_supprimer = (Utxo *)courant->info;
 
        if (utxo_a_supprimer != NULL) {
            //on libère d'abord le billet pointé par l'étiquette
            if (utxo_a_supprimer->txOut != NULL) {
                //on libère le nom (alloué par strdup)
                if (utxo_a_supprimer->txOut->lockingScript[0] != NULL) {
                    free(utxo_a_supprimer->txOut->lockingScript[0]);
                }
                free(utxo_a_supprimer->txOut);
            }
            //on libère ensuite l'étiquette elle-même
            free(utxo_a_supprimer);
        }
 
        //libère le noeud de la liste chainée
        free(courant);
 
        courant = suivant;
    }
 
    // retour du pointeur à NULL
    global_utxo_list = NULL;
}