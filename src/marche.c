#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "define.h"
#include "utxo.h"
#include "blockchain.h"
#include "sha256_utils.h"

//variables globales
static int cycleRounds = 0;
static const int limit = 30; // Nombre de blocs avant la division par 2
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
 * Point 9 : Calculer les frais de minage
 * Total_frais = Somme(Inputs) - Somme(Outputs)
 */
long calculer_frais_bloc(Blockchain *bc, Block *bloc) {
    long total_frais = 0;
    
    if (bloc == NULL || bloc->transactions == NULL) {
        return 0;
    }

    Slist *courant = bloc->transactions;
    while (courant != NULL) 
    {
        Transaction *tx = (Transaction *)courant->info;
        
        if (tx != NULL) 
        {
            //On ignore la transaction coinbase s'il y en a deja une
            if (strcmp((char*)tx->adSender, "SYSTEM_COINBASE") == 0) 
            {
                courant = courant->next;
                continue;
            }

            // Somme(Inputs)
            long sum_inputs = 0;
            Slist *input = tx->lstInputs;
            while (input != NULL) 
            {
                TxInputs *txIn = (TxInputs *)input->info;
                if (txIn != NULL) 
                {
                    sum_inputs += get_input_amount(bc, (char*)txIn->txHash, txIn->indexOutput);
                }
                input = input->next;
            }

            // Somme(Outputs)
            long sum_outputs = 0;
            Slist *output = tx->lstOutputs;
            while (output != NULL) 
            {
                TxOutputs *txOut = (TxOutputs *)output->info;
                if (txOut != NULL) 
                {
                    sum_outputs += txOut->amount;
                }
                output = output->next;
            }

            // L'argent 'restant' (différence in/out) correspond aux frais laisses par l'utilisateur
            if (sum_inputs > sum_outputs) 
            {
                total_frais += (sum_inputs - sum_outputs);
            }
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
        printf("  La recompense de minage est divisee par 2 : %d BT\n", bc->reward4mining);
        printf("=========================================================\n\n");
    }

    // Point 13 : Retourne 0 quand l'inflation est termine 
    if (bc->reward4mining == 0) 
    {
        printf("\n--- FIN DE LA PHASE D'INFLATION ---\n");
        printf("Il n'y a plus de creation monetaire, seuls les frais retribueront le mineur.\n");
    }

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
    long frais = calculer_frais_bloc(bc, bloc);
    
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

    printf("=> TX Coinbase (1ere position) pour %s : %ld BT (Recompense: %ld, Frais: %ld)\n", 
            miner_address, montant_total, recompense, frais);

    // Point 13 : Détecter la fin de l'inflation et afficher le bilan final
    if (recompense == 0 && (cycleRounds % limit) == 0) {
        printf("\n--- BILAN FINAL D'INFLATION ---\n");
        printf("Masse monetaire totale  : %ld BT\n", currency->moneySupply);
        printf("Nombre de cycles Halving: %d cycles\n", nb_halvings);
        printf("---------------------------------------------------------\n\n");
    }
}