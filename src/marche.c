#include <unistd.h>      
#include "utxo.h"        
#include "transaction.h"
#include "marche.h"
#include <signal.h>

// Finalise une transaction incomplète 
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
    
    //  Destinataire
    TxOutputs *out_dest = creer_output(tx->txAmount, (char*)tx->adReceiver);
    tx->lstOutputs = inserer_en_tete(NULL, out_dest);
    ajouter_utxo(out_dest, (char*)tx->txid, 0); // Index 0

    //  Change (si nécessaire)
    if (monnaie_rendue > 0) {
        TxOutputs *out_change = creer_output(monnaie_rendue, donneur->str);
        tx->lstOutputs = inserer_en_tete(tx->lstOutputs, out_change);
        ajouter_utxo(out_change, (char*)tx->txid, 1); // Index 1
        tx->nbOutputs = 2;
    } else {
        tx->nbOutputs = 1;
    }

    // Calcul du Hash Final (TXID)
    char buffer[MAX_BUF];
    sprintf(buffer, "%s%s%ld%ld", tx->adSender, tx->adReceiver, tx->txAmount, tx->timestamp);
    sha256ofString((BYTE *)buffer, (char*)tx->txid);
}

void lancer_phase_marche(Blockchain *bc, ListeAccounts *la, currency_t *curr_info) {
    int cycleRounds = 0;
    extern volatile sig_atomic_t pause_flag; // Déclarée dans le main

    printf("Démarrage de la Phase de Marché... (Ctrl+C pour Pause)\n");

    while (bc->reward4mining > 0) { // Arrêt si récompense == 0
        
        // --- PAUSE (Ctrl+C) ---
        if (pause_flag) {
            printf("\n--- PAUSE ---\n");
            printf("Masse Monétaire : %ld BT\n", curr_info->moneySupply);
            printf("Récompense actuelle : %d BT\n", bc->reward4mining);
            printf("Cycles effectués : %d\n", cycleRounds);
            
            printf("1. Reprendre | 2. Quitter : ");
            int choix;
            scanf("%d", &choix);
            if (choix == 2) break;
            pause_flag = 0;
            printf("Reprise...\n");
        }

        //  Générer des transactions aléatoires 
        for (int i = 0; i < 3; i++) { // On en génère 3 par round
            generer_transaction_aleatoire(la);
        }

        //  Gestion du Halving 
        if (bc->nbBlocks > 0 && bc->nbBlocks % 10 == 0) {
            bc->reward4mining /= 2;
            printf("\n>>> HALVING ! Nouvelle récompense : %d BT <<<\n", bc->reward4mining);
        }

        cycleRounds++;
        usleep(500000); // Petite pause de 0.5s 
    }

    printf("\n--- FIN ---\n");
    printf("Total Blocs : %d | Cycles : %d | Masse : %ld\n", bc->nbBlocks, cycleRounds, curr_info->moneySupply);
}