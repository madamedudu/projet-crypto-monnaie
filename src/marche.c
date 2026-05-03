#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include "define.h"
#include "utxo.h"
#include "blockchain.h"
#include "sha256_utils.h"
#include "transaction.h"
#include "marche.h"
#include "bloc.h"

//variables globales
static int cycleRounds = 0;
static const int limit = HALVING;
static int nb_halvings = 0;  // Pour le bilan final

/* 
 *  Trouver le montant d'un UTXO consommé (pour calculer les frais) 
 */
long get_input_amount(Blockchain *bc, char *txHash, int index) {
    if (bc == NULL || bc->blocklist == NULL) {
        return 0;
    }
        
    // On parcourt toute la blockchain pour retrouver la transaction source
    Slist *bloc_courant = bc->blocklist;
    while (bloc_courant != NULL) 
    {
        Block *b = (Block *)bloc_courant->info;
        if (b != NULL) 
        {
            Slist *tx_courante = b->transactions;
            while (tx_courante != NULL) 
            {
                Transaction *tx = (Transaction *)tx_courante->info;
                // Si la TX source est trouve
                if (tx != NULL && strcmp((char*)tx->txid, txHash) == 0) 
                {
                    Slist *out_courant = tx->lstOutputs;
                    int current_idx = 0;
                    while (out_courant != NULL) 
                    {
                        TxOutputs *out = (TxOutputs *)out_courant->info;
                        if (out != NULL && current_idx == index) 
                        {
                            return out->amount; // On retourne le montant original
                        }
                        current_idx++;
                        out_courant = out_courant->next;
                    }
                }
                tx_courante = tx_courante->next;
            }
        }
        bloc_courant = bloc_courant->next;
    }
    return 0; 
}

/* 
 * Point 9 : Calculer les frais de minage depuis outputs
 */
long calculer_frais_bloc(Block *bloc) {
    long total_frais = 0;

    if (bloc == NULL || bloc->transactions == NULL){
        return 0;
    } 

    Slist *courant = bloc->transactions;
    while (courant != NULL) {
        Transaction *tx = (Transaction *)courant->info;

        if (tx != NULL && strcmp((char*)tx->adSender, "SYSTEM_COINBASE") != 0) {
            // Les frais = txAmount * FEE_RATE / 100, déjà calculés à la finalisation
            total_frais += (tx->txAmount * FEE_RATE) / 100;
        }
        courant = courant->next;
    }

    return total_frais;
}

/* 
 * Point 8, 12, 13 : Recompense, Halving et Bilan Inflation
 *  */
long calculer_recompense(Blockchain *bc) {
    if (bc == NULL) {
        return 0;
    }

    cycleRounds++; // Incremente a chaque nouveau bloc mine

    // Point 12 : Diviser la recompense par 2 quand cycleRounds % limit == 0 
    if (cycleRounds > 0 && (cycleRounds % limit) == 0) 
    {
        bc->reward4mining /= 2;
        nb_halvings++;
        printf("\n=========================================================\n");
        printf("  [CYCLE HALVING] %d blocs mines !\n", cycleRounds);
        printf("  La recompense de minage est divisee par 2 : %d BTU\n", bc->reward4mining);
        printf("=========================================================\n\n");
    }

    // Point 13 : Retourne 0 quand l'inflation est termine 
    

    return bc->reward4mining;
}

/* 
 * Point 9 and 11 : Transaction Coinbase et Mise à jour Monnaie
 */
void creer_tx_coinbase(currency_t *currency, Block *bloc, char *miner_address) {
    if (currency == NULL || currency->bc == NULL || bloc == NULL || miner_address == NULL) {
        return;
    }

    Blockchain *bc = currency->bc;

    // 1. Calculer la recompense : salaire
    long recompense = calculer_recompense(bc);
    
    // 2. Calculer les frais : pourboires laissés dans le bloc
    long frais = calculer_frais_bloc( bloc);
    
    long montant_total = recompense + frais;

    // S'il n'y a ni recompense (fin inflation) ni frais (bloc vide ou tx sans frais), on ne genere rien
    if (montant_total <= 0) {
        return;
    }
    // 3. Créer la transaction speciale (sans émetteur)
    Transaction *tx_coinbase = malloc(sizeof(Transaction));
    if (tx_coinbase == NULL) {
        printf("Erreur allocation TX Coinbase\n");
        return;
    }
    memset(tx_coinbase, 0, sizeof(Transaction));

    strncpy((char*)tx_coinbase->adSender, "SYSTEM_COINBASE", HASHLENGTH);
    strncpy((char*)tx_coinbase->adReceiver, miner_address, HASHLENGTH);
    tx_coinbase->txAmount = montant_total;
    tx_coinbase->timestamp = time(NULL);
    strncpy(tx_coinbase->comment, "Recompense de minage + Frais des transactions", MAX_STRING);

    // Pas de input (creation ex-nihilo)
    tx_coinbase->nbInputs = 0;
    tx_coinbase->lstInputs = NULL;

    // 1 output vers le mineur = récompense + frais
    TxOutputs *out_coinbase = creer_output(montant_total, miner_address);
    out_coinbase->outIndex = 0;
    
    Slist *noeud_out = malloc(sizeof(Slist));
    noeud_out->info = out_coinbase;
    noeud_out->next = NULL;
    
    tx_coinbase->lstOutputs = noeud_out;
    tx_coinbase->nbOutputs = 1;

    // Calcul du TXID (Hash)
    char buffer[MAX_BUF];
    sprintf(buffer, "%s%s%ld%ld", tx_coinbase->adSender, tx_coinbase->adReceiver, montant_total, tx_coinbase->timestamp);
    sha256ofString((BYTE *)buffer, (char*)tx_coinbase->txid);

    // Ajout de Output à la liste des UTXO Globaux
    ajouter_utxo(out_coinbase, (char*)tx_coinbase->txid, 0);

    // 4. Insérer en 1ère position du bloc 
    Slist *noeud_tx = malloc(sizeof(Slist));
    noeud_tx->info = tx_coinbase;
    noeud_tx->next = bloc->transactions; // L'ancien premier devient le deuxième
    bloc->transactions = noeud_tx;       // Le coinbase devient le nouveau premier
    bloc->nbTx++;

    // 5. Faire moneySupply += Recompense (Point 11 - Ajouter uniquement le salaire créé, pas les frais )
    currency->moneySupply += recompense;

    printf("=> TX Coinbase (1ere position) pour %s : %ld BTU (Recompense: %ld, Frais: %ld)\n", 
            miner_address, montant_total, recompense, frais);

    // Point 13 : Détecter la fin de l'inflation et afficher le bilan final
    
}

// ----------Finalise une transaction incomplète --------------------
void finaliser_transaction_par_mineur(Transaction *tx, Account *donneur) {
    if (tx == NULL) return;

    //Calcul du montant total des inputs pour définir le change
    long total_entree = 0;
    Slist *curr = tx->lstInputs;
    while (curr) {
        Utxo *u = (Utxo*)curr->info;
        total_entree += u->txOut->amount;
        curr = curr->next;
    }

    //  Calcul des frais (5%) et du change
    long frais = (tx->txAmount * FEE_RATE) / 100;
    long monnaie_rendue = total_entree - tx->txAmount - frais;

    //  Création des Outputs (Paiement + Change)
    tx->timestamp = time(NULL);


    // Calcul du Hash(TXID) il doit être connu avant d'enregistrer les outputs dans global_utxo_list
    char buffer[MAX_BUF];
    sprintf(buffer, "%s%s%ld%ld", tx->adSender, tx->adReceiver, tx->txAmount, tx->timestamp);
    sha256ofString((BYTE *)buffer, (char*)tx->txid);

    //"modif chirine" suppr des utxo consommes
    curr = tx->lstInputs;
    while (curr) {
        Utxo *u = (Utxo*)curr->info;
        supprimer_utxo((char*)u->hash, u->indexOutput);
        curr = curr->next;
    }

    tx->nbOutputs = 0;
    tx->lstOutputs = NULL;
    //  Destinataire
    TxOutputs *out_dest = creer_output(tx->txAmount, (char*)tx->adReceiver);
    out_dest->outIndex = 0;
    tx->lstOutputs = inserer_en_queue(tx->lstOutputs, out_dest);
    ajouter_utxo(out_dest, (char*)tx->txid, 0);
    tx->nbOutputs++;

    //frais output 1
    TxOutputs *out_frais = creer_output(frais, "FEES");
    out_frais->outIndex = 1;
    tx->lstOutputs = inserer_en_queue(tx->lstOutputs, out_frais);
    tx->nbOutputs++;

    //change (genre la monnaie rendu)
    if (monnaie_rendue > 0) {
        TxOutputs *out_change = creer_output(monnaie_rendue, donneur->str);
        out_change->outIndex = 2;
        tx->lstOutputs = inserer_en_queue(tx->lstOutputs, out_change);
        ajouter_utxo(out_change, (char*)tx->txid, 2);
        tx->nbOutputs++;;
    }

}


//--------------------------------------------------------
//lancement phase marché
//---------------------------------------------------------

void lancer_phase_marche(Blockchain *bc, ListeAccounts *la, currency_t *curr_info) {
    extern volatile sig_atomic_t pause_flag; // Déclarée dans le main

    printf("Démarrage de la Phase de Marché... (Ctrl+C pour Pause)\n");
    int bilan_affiche = 0; //affichage du bilan 1 fois
    while (1) { // boucle infinie
        
        // --- PAUSE (Ctrl+C) ---
        if (pause_flag) {
            printf("\n--- PAUSE ---\n");
            printf("Masse Monétaire : %ld BTU\n", curr_info->moneySupply);
            printf("Récompense actuelle : %d BTU\n", bc->reward4mining);
            printf("Cycles effectués : %d\n", cycleRounds);
            
            printf("1. Reprendre | 2. Quitter : ");
            int choix;
            scanf("%d", &choix);
            if (choix == 2) break;
            pause_flag = 0;
            printf("Reprise...\n");
        }
        if (bc->reward4mining == 0 && !bilan_affiche) {
        printf("\n=== FIN DE L'INFLATION ===\n");
        printf("Masse monétaire finale : %ld BTU\n", curr_info->moneySupply);
        printf("Plus de création monétaire, la blockchain continue avec les frais uniquement.\n");
        bilan_affiche = 1;
        sleep(2);//on voit que l'inflation s'arrete
        }

        // générer des transactions aléatoires pr remplir bloc
        int n = (rand() % MAXTX) + 1;
        for (int i = 0; i < n; i++) { 
            generer_transaction_aleatoire(la);
        }

        usleep(500000); // Petite pause de 0.5s 

        //tirer un mineur au sort
        int idx_mineur = rand() % la->nb_accounts;
        Account *mineur = &la->accounts[idx_mineur];
        printf("Mineur du cycle : %s\n", mineur->str);

        //creer bloc temporaire
        Block *bloc_a_miner = create_nouveau_bloc(bc);
        if (bloc_a_miner == NULL) {
            printf("[Erreur] Impossible de créer le bloc.\n");
            break;
        }
        strncpy(bloc_a_miner->minerName, mineur->str, MAX_STRING);
        strncpy(bloc_a_miner->comment, "Bloc phase marche", MAX_STRING);

        //finaliser chaque transaction
        int tx_ajoutees = 0;
        printf("Le mineur rassemble les transactions du memory pool...\n");
        
        while (tx_ajoutees < MAXTX-1) {//une place reservée pr coinbase
            Transaction *tx_piochee = defiler_mempool();
            if (tx_piochee == NULL) {
                break; // Mempool vide, on arrête de piocher
            }

            Account *donneur = trouver_compte(la, (char*)tx_piochee->adSender);
            if (donneur != NULL) {
                finaliser_transaction_par_mineur(tx_piochee, donneur);
                //on ajoute directement au bloc temporaire
                bloc_a_miner->transactions = inserer_en_tete(bloc_a_miner->transactions, tx_piochee);
                bloc_a_miner->nbTx++;
                tx_ajoutees++;
            } else {
                free(tx_piochee); // donneur introuvable, on libère
            }
        
        }

        //creation transac coinbase
        creer_tx_coinbase(curr_info, bloc_a_miner, mineur->str);

        //minage du bloc
        printf("Minage du bloc #%d...\n", bloc_a_miner->index);
        mine_block(bloc_a_miner, bc->difficulty);


        //vérification bloc et ajout
        printf("Vérification de l'intégrité du bloc #%d...\n", bloc_a_miner->index);
        if (!ajouter_bloc_blockchain(bc, bloc_a_miner, bc->difficulty)) {
            printf("[Erreur] Bloc #%d rejeté.\n", bloc_a_miner->index);
            free(bloc_a_miner);
        } else {
            printf("Cycle terminé | Blocs : %d | Masse : %ld BTU\n",bc->nbBlocks, curr_info->moneySupply);
        }
        printf(("\n"));

    }

    printf("\n--- FIN ---\n");
    printf("Total Blocs : %d | Cycles : %d | Masse : %ld\n", bc->nbBlocks, cycleRounds, curr_info->moneySupply);
}