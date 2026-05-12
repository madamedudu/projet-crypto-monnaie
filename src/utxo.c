#include <stdio.h>
#include <stdlib.h> 
#include "define.h"
#include "utxo.h"
#include "transaction.h"



//voici la liste global
struct Slist *global_utxo_list = NULL;

//-------------CREATION D'UN OUTPUT-----------------------
TxOutputs* creer_output(long montant, char *nom_destinataire) {
    if (nom_destinataire == NULL) return NULL;
 
    //allocation mémoire du nouveau billet
    TxOutputs *nouveau_output = malloc(sizeof(TxOutputs));
    if (nouveau_output == NULL) {
        printf("[Erreur] Allocation memoire echouee pour l'output.\n");
        return NULL;
    }
 
    nouveau_output->amount = montant;
    nouveau_output->outIndex = 0;
    nouveau_output->timestamp = (long)time(NULL);
 
    //on stocke le nom du proprio avec strdup (et on vérifie que ça marche)
    nouveau_output->lockingScript[0] = strdup(nom_destinataire);
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

// L'algorithme glouton
Slist* select_utxos_greedy(char *nom_emetteur, long montant_cible, long *somme_recuperee) {
    if (global_utxo_list == NULL || montant_cible <= 0) return NULL;

    Utxo *best_greater = NULL;
    long min_greater_val = -1;
    Slist *selection = NULL;
    *somme_recuperee = 0;

    // Chercher le "Smallest Greater" (un seul billet qui suffit)
    struct Slist *courant = global_utxo_list;
    while (courant != NULL) {
        Utxo *u = (Utxo *)courant->info;
        if (u->txOut != NULL && strcmp(u->txOut->lockingScript[0], nom_emetteur) == 0) {
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
        if (u->txOut != NULL && strcmp(u->txOut->lockingScript[0], nom_emetteur) == 0) {
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
    struct Slist *curr = global_utxo_list; // ← parcourir la liste GLOBALE
    while (curr != NULL) {
        Utxo *u = (Utxo *)curr->info;
        if (u && u->txOut && u->txOut->lockingScript[0] &&
            strcmp(u->txOut->lockingScript[0], acc->str) == 0) {
            total += u->txOut->amount;
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