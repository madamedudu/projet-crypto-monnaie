#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include "define.h"
#include "sha256_utils.h"
#include "blockchain.h"   
#include "bloc.h"
#include "transaction.h"
#include "utils.h" 
#include "utxo.h" 
#include "cryptographie.h"
#include "marche.h" 
#include "script.h"

// Variable globale pour la pause (CTRL C)
volatile sig_atomic_t pause_flag = 0;
void handle_sigint(int sig) {
    (void)sig;
    pause_flag = 1; // On lève le drapeau de pause
}


void tester_scripts() {
    printf("\n=== TEST DE CREATION DES SCRIPTS ===\n");
    
    //réparation des variables fausses
    TxOutputs fausse_sortie;
    TxInputs fausse_entree;
    char buffer_affichage[256]; // Pour stocker la chaîne finale

    // 2. Test du Lock Script (Verrouillage)
    creer_lock_script(&fausse_sortie, "SIGNATURE_FAUSSE", "CLE_PUB_FAUSSE");
    
    // On utilise ta fonction script_to_string pour lire les 4 cases
    script_to_string(fausse_sortie.lockingScript, 4, buffer_affichage, sizeof(buffer_affichage));
    
    printf("-> Lock Script généré : '%s'\n", buffer_affichage);
    // Attendu : "SIGNATURE_FAUSSE CLE_PUB_FAUSSE DUP HASH"


    // 3. Test du Unlock Script (Déverrouillage)
    creer_unlock_script(&fausse_entree, "HASH_CLE_PUB");
    
    // On utilise ta fonction script_to_string pour lire les 3 cases
    script_to_string(fausse_entree.unlockingScript, 3, buffer_affichage, sizeof(buffer_affichage));
    
    printf("-> Unlock Script généré : '%s'\n", buffer_affichage);
    // Attendu : "HASH_CLE_PUB EQ VER"
    
    printf("====================================\n\n");
}
// Dans main() avant de lancer le marché pour brancher l'interruption CTRL C: signal(SIGINT, handle_sigint);

//on essaye de voir si ça git pull ce main
int main() {
    printf("\n========== DEMARRAGE SYSTEME BLOCKCHAIN ==========\n");

    currency_t *ma_monnaie = NULL;
    ListeAccounts ma_liste_accounts;
    ma_liste_accounts.nb_accounts = 0;
    int helicopter_deja_lance = 0; //faux pour l'instant
    int choix = 0;
    srand(time(NULL));
    int tx_en_attente = 0; //compteur pr créer un bloc manuellement
    int tx_max_bloc = (rand() % (MAXTX - 1)) + 1; // entre 1 et 9

    // Installation du handler de signal
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;         // pas SA_RESTART : on veut interrompre les appels bloquants
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }
    do {
        printf("\n================ MENU PRINCIPAL ================\n");
        printf("1. Creer la blockchain, la genesis, les utilisateurs\n");
        printf("2. Lancer l'helicopter money (Distribution initiale)\n");
        printf("3. Creer une transaction\n");
        printf("4. Sauver la blockchain au format json\n");
        printf("5. Lancer la phase de marché\n"); //phase dynamique Verifier la coherence de la blockchain
        printf("6. Afficher les utilisateurs\n");
        printf("7. Consulter registre utxo\n");
        printf("8. Reconstituer le wallet d'une adresse\n");
        printf("9. Quitter\n");
        printf("================================================\n");
        
        int saisie_valide = 0;
        while (saisie_valide == 0) {
            printf("Votre choix (1-9) : ");
            
            //gestion de l'erreur si l'utilisateur tape une lettre
            if (scanf("%d", &choix) != 1) {
                while(getchar() != '\n'); 
                printf("[Erreur] Veuillez entrer un chiffre entier.\n");
            } 
            //gestion de l'erreur si le chiffre est hors limites (1 a 9)
            else if (choix < 1 || choix > 9) {
                printf("[Erreur] Veuillez entrer un chiffre entre 1 et 9.\n");
            } 
            else {
                saisie_valide = 1; 
            }
        }

        switch (choix) {
            
            case 1:
            //MODIFICATIONS POUR ACCOUNTS
                printf("\n--- INITIALISATION BLOCKCHAIN ET UTILISATEURS ---\n");
                if (ma_monnaie != NULL) {
                    printf("[Info] La blockchain existe deja.\n");
                    break;
                }
                
                ma_monnaie = init_currency();
                
                int nb_comptes = MAX_USERS;
                printf("Nombre d'utilisateurs %d\n",nb_comptes );
                
                ma_liste_accounts = generer_accounts(nb_comptes);
                
                if (ma_liste_accounts.nb_accounts > 0) {
                    printf("[Succes] %d utilisateurs crees !\n", ma_liste_accounts.nb_accounts);
                }
                break;

            case 2:
            printf("\n--- HELICOPTER MONEY ---\n");
                if (ma_monnaie == NULL ||ma_liste_accounts.nb_accounts == 0) {
                    printf("[Erreur] Veuillez d'abord creer la blockchain (Option 1).\n");
                } 
                else if (helicopter_deja_lance == 1) {
                    printf("[Erreur] L'Helicopter Money a déjà été distribué.\n");
                } 
                else {
                    
                    run_helicopter_money(&ma_liste_accounts, ma_monnaie);
                    helicopter_deja_lance = 1; 
                    printf("[Succes] Distribution de l'helicopter money realisee.\n\n");
                    afficher_accounts(ma_liste_accounts);
                }
                break;
                

            case 3: {
                printf("\n--- NOUVELLE TRANSACTION INTERACTIVE ---\n");
                if (ma_monnaie == NULL || ma_liste_accounts.nb_accounts == 0 || helicopter_deja_lance == 0) {
                    printf("[Erreur] Initialisez l'option 1 et l'option 2 d'abord.\n");
                    break;
                }

                // 1. Affichage des utilisateurs disponibles
                printf("\n-- Utilisateurs enregistres --\n");
                for (int i = 0; i < ma_liste_accounts.nb_accounts; i++) {
                    printf("%2d) %-8s | adresse=%s | pub=%.10s... | solde=%ld BTU\n",
                           i + 1,
                           ma_liste_accounts.accounts[i].str,
                           ma_liste_accounts.accounts[i].address,
                           ma_liste_accounts.accounts[i].pub_key,
                           calculer_solde_reel(&ma_liste_accounts.accounts[i]));
                }
                printf("------------------------------\n");

                // 2. Saisie des informations
                char emetteur[ADDRESS_LEN], beneficiaire[ADDRESS_LEN], dest_address[ADDRESS_LEN];
                long montant;

                printf("Entrez l'adresse ou l'index de l'emetteur : "); 
                scanf("%34s", emetteur);
                
                printf("Entrez l'adresse ou l'index du beneficiaire : "); 
                scanf("%34s", beneficiaire);
                
                printf("Entrez le montant UTXO : ");
                if (scanf("%ld", &montant) != 1 || montant <= 0) {
                    while(getchar() != '\n');
                    printf("[Erreur] Montant invalide.\n");
                    break;
                }

                // 3. Identification des comptes (par Index ou par Adresse)
                struct account *src = NULL, *dst = NULL;
                int idx_src = atoi(emetteur), idx_dst = atoi(beneficiaire);

                // Traitement Emetteur
                if (idx_src >= 1 && idx_src <= ma_liste_accounts.nb_accounts) src = &ma_liste_accounts.accounts[idx_src - 1];
                else src = trouver_compte_par_adresse(&ma_liste_accounts, emetteur);

                // Traitement Beneficiaire
                if (idx_dst >= 1 && idx_dst <= ma_liste_accounts.nb_accounts) dst = &ma_liste_accounts.accounts[idx_dst - 1];
                else dst = trouver_compte_par_adresse(&ma_liste_accounts, beneficiaire);

                if (src == NULL || dst == NULL) {
                    printf("[Erreur] L'emetteur ou le beneficiaire est introuvable.\n");
                    break;
                }

                strncpy(dest_address, (char*)dst->address, ADDRESS_LEN);
                dest_address[ADDRESS_LEN - 1] = '\0';

                // 4. Verification de la faisabilite et creation
                if (calculer_solde_reel(src) < montant) {
                    printf("[Erreur] Montant insuffisant. Solde de %s : %ld BTU\n", src->str, src->balance);
                    break;
                }

                Transaction *tx = create_incomplete_transaction(src, dest_address, montant);
                if (tx != NULL) {
                    finaliser_transaction_par_mineur(tx, src, &ma_liste_accounts); //on signe et on ajoute les inputs manquants
                    enfiler_mempool(tx);
                    tx_en_attente++;
                    
                    printf("[Succes] Transaction %s -> %s (%ld BTU) ajoutée au mempool (%d/%d).\n",
                           (char*)src->address, dest_address, montant, tx_en_attente, tx_max_bloc);
                    
                    // 5. Minage automatique si mempool plein
                    if (tx_en_attente >= tx_max_bloc) {
                        printf("\n[Info] Mempool plein, minage automatique...\n");
                        int idx_mineur = rand() % ma_liste_accounts.nb_accounts;
                        Account *mineur_acc = &ma_liste_accounts.accounts[idx_mineur];
                        
                        printf("[Info] Mineur tire au sort : %s\n", mineur_acc->str);

                        Block *bloc = create_nouveau_bloc(ma_monnaie->bc);
                        if (bloc == NULL) break;
                        
                        strncpy(bloc->minerName, mineur_acc->str, MAX_STRING);
                        strncpy(bloc->comment, "Bloc transactions manuelles", MAX_STRING);

                        int nb = 0;
                        while (nb < tx_max_bloc) {
                            Transaction *tx_piochee = defiler_mempool();
                            if (tx_piochee == NULL) break;
                            bloc->transactions = inserer_en_tete(bloc->transactions, tx_piochee);
                            bloc->nbTx++;
                            nb++;
                        }
                        
                        creer_tx_coinbase(ma_monnaie, bloc, mineur_acc);
                        mine_block(bloc, ma_monnaie->bc->difficulty);

                        if (!ajouter_bloc_blockchain(ma_monnaie->bc, bloc, ma_monnaie->bc->difficulty)) {
                            printf("[Erreur] Bloc rejeté.\n");
                            liberer_bloc_rejete(bloc);
                        } else {
                            printf("[Succes] Bloc #%d miné avec %d transactions.\n", bloc->index, bloc->nbTx);
                        }
                        
                        tx_en_attente = 0;
                        tx_max_bloc = (rand() % (MAXTX - 1)) + 1; 
                    }
                } else {
                    printf("[Erreur] Probleme lors de la selection des UTXO (Fonds fragmentes ou script invalide).\n");
                }
                break;
            }

            case 4:
                printf("\n--- EXPORT JSON ---\n");
                if (ma_monnaie == NULL) {
                    printf("[Erreur] Veuillez d'abord creer la blockchain (Option 1).\n");
                } else {
                    char nom_fichier[100];
                    printf("Nom du fichier de sauvegarde : ");
                    scanf("%99s", nom_fichier);
                    int len = strlen(nom_fichier);
                    
                    // Si le mot est trop court ou s'il ne se termine pas par ".json"
                    if (len < 5 || strcmp(nom_fichier + len - 5, ".json") != 0) {
                        strcat(nom_fichier, ".json");
                    }

                    export_blockchain_json(ma_liste_accounts, ma_monnaie->bc, nom_fichier);
                }
                break;

            case 5:
                
                printf("\n--- Phase de Marché ---\n");
                if (ma_monnaie == NULL || ma_liste_accounts.nb_accounts == 0|| helicopter_deja_lance == 0) {
                    printf("[Erreur] Veuillez d'abord initialiser la blockchain (Option 1) et l'helicopter money (Option 2).\n");
                    break;
                }
                pause_flag = 0;
                lancer_phase_marche(ma_monnaie->bc, &ma_liste_accounts, ma_monnaie);
                break;
            
            case 6:
                
                if (ma_liste_accounts.nb_accounts == 0) {
                    printf("[Info] Aucun utilisateur n'est enregistré.\n");
                } else {
                    afficher_accounts(ma_liste_accounts);
                }
                break;
            
            case 7: // NOUVELLE OPTION
                printf("\n--- CONSULTATION DU REGISTRE UTXO ---\n");
                if (global_utxo_list == NULL) {
                    printf("[Info] Le registre est vide. Lancez l'Helicopter Money d'abord.\n");
                } else {
                    afficher_utxo_global();
                }
                break;

            case 8:
                printf("\n--- RECONSTITUTION DU WALLET ---\n");
                if (ma_monnaie == NULL || ma_monnaie->bc == NULL) {
                    printf("[Erreur] Veuillez d'abord creer la blockchain (Option 1).\n");
                } else {
                    printf("\n-- Adresses disponibles --\n");
                    for (int i = 0; i < ma_liste_accounts.nb_accounts; i++) {
                        printf("%2d) %-8s | adresse=%s | pub=%.10s...\n",
                               i + 1,
                               ma_liste_accounts.accounts[i].str,
                               ma_liste_accounts.accounts[i].address,
                               ma_liste_accounts.accounts[i].pub_key);
                    }
                    printf("------------------------------\n");

                    char adresse[ADDRESS_LEN];
                    printf("Adresse Base58 : ");
                    scanf("%34s", adresse);

                    Slist *wallet = reconstruire_wallet(ma_monnaie->bc, adresse);
                    if (wallet == NULL) {
                        printf("Aucun UTXO non depense pour cette adresse.\n");
                    } else {
                        printf("UTXO non depenses pour %s :\n", adresse);
                        int count = 0;
                        Slist *courant = wallet;
                        while (courant != NULL) {
                            Utxo *u = (Utxo *)courant->info;
                            if (u != NULL && u->txOut != NULL) {
                                printf("[%d] TXID %.16s... | index %d | montant %ld BTU\n",
                                       count,
                                       (char *)u->hash,
                                       u->indexOutput,
                                       u->txOut->amount);
                                count++;
                            }
                            courant = courant->next;
                        }
                        printf("Total UTXO : %d\n", count);

                        while (wallet != NULL) {
                            Slist *tmp = wallet;
                            free(tmp->info);
                            wallet = wallet->next;
                            free(tmp);
                        }
                    }
                }
                break;

            
            case 9:
                //netoyage de tout l'espace en memoire
                vider_mempool(); //libere les transactions encore dans le mempool
                if (ma_monnaie != NULL) {
                    liberer_blockchain(ma_monnaie->bc); //libere blocs, tx, outputs, lockingScript
                    free(ma_monnaie);
                }
                liberer_registre_utxo(); //libere les noeuds du registre (txOut deja liberes au dessus)
                liberer_accounts(&ma_liste_accounts); //libere le tableau de comptes
                printf("\nFermeture du programme.\n");
                return 0;
        }

        //affichage de la masse monetaire en fin de chaque boucle
        if (ma_monnaie != NULL) {
            printf("\n------------------------------------------------\n");
            printf("Masse monetaire totale : %ld BTU\n", ma_monnaie->moneySupply);
            printf("------------------------------------------------\n");
        }

    } while (choix != 9);

    return 0;
}