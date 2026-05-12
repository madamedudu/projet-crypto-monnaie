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
#include "marche.h" 

// Variable globale pour la pause (CTRL C)
volatile sig_atomic_t pause_flag = 0;
void handle_sigint(int sig) {
    (void)sig;
    pause_flag = 1; // On lève le drapeau de pause
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
        printf("8. Quitter\n");
        printf("================================================\n");
        
        int saisie_valide = 0;
        while (saisie_valide == 0) {
            printf("Votre choix (1-8) : ");
            
            //gestion de l'erreur si l'utilisateur tape une lettre
            if (scanf("%d", &choix) != 1) {
                while(getchar() != '\n'); 
                printf("[Erreur] Veuillez entrer un chiffre entier.\n");
            } 
            //gestion de l'erreur si le chiffre est hors limites (1 a 6)
            else if (choix < 1 || choix > 8) {
                printf("[Erreur] Veuillez entrer un chiffre entre 1 et 8.\n");
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
                

            case 3:
                //transaction entre utilisateurs
                printf("\n--- NOUVELLE TRANSACTION ---\n");
                if (ma_monnaie == NULL || ma_liste_accounts.nb_accounts == 0|| helicopter_deja_lance == 0) {
                    printf("[Erreur] Veuillez d'abord creer la blockchain et les utilisateurs (Option 1) et lancer l'hélicoptère money (Option 2).\n");
                    break;
                }

                //affiche la liste des users
                printf("\n-- Utilisateurs enregistres --\n");
                for (int i = 0; i < ma_liste_accounts.nb_accounts; i++) {
                    printf("- %s (Solde : %ld BT)\n", ma_liste_accounts.accounts[i].str, ma_liste_accounts.accounts[i].balance); //modification
                }
                printf("------------------------------\n");

                char emetteur[MAX_STRING];
                char beneficiaire[MAX_STRING];
                long montant;

                printf("Entrez le nom de l'emetteur : ");
                scanf("%63s", emetteur);
                
                printf("Entrez le nom du beneficiaire : ");
                scanf("%63s", beneficiaire);
                
                //a faire
                printf("Entrez le montant UTXO : ");
                if (scanf("%ld", &montant) != 1) {
                    while(getchar() != '\n');
                    printf("Montant invalide.\n");
                    break;
                }
                if (montant <= 0) {
                    printf("Le montant doit etre strictement positif.\n");
                    break;
                }



                //on verifie si le nom de src et dst est dans la liste des utilisateurs
                struct account *src= trouver_compte(&ma_liste_accounts, emetteur);
                if(src==NULL){
                    printf("L'émétteur entrée est erroné, veuillez refaire la transaction.\n");
                    break;
                }
                struct account *dst= trouver_compte(&ma_liste_accounts,beneficiaire);
                if(dst==NULL){
                    printf("Le bénéficiaire entrée est erroné, veuillez refaire la transaction.\n");
                    break;
                }


                //test du montant
                if (calculer_solde_reel(src)<montant) {
                    printf("Le montant est insuffisant, solde de %s : %ld \n",src->str, src->balance);
                    break;
                }
                
                
                //mise en place de la transaction  (similaire phase de marché mais cas unique)
                Transaction *tx = create_incomplete_transaction(src, beneficiaire, montant);
                
                if (tx != NULL) {
                    finaliser_transaction_par_mineur(tx, src); //on maj les soldes et utxo
                    enfiler_mempool(tx);
                    tx_en_attente++;
                    
                    printf("[Succes] Transaction %s -> %s (%ld BTU) ajoutée au mempool (liste d'attente) (%d/%d).\n",emetteur, beneficiaire, montant, tx_en_attente, tx_max_bloc);
                    
                    //si le mempool est plein, on mine automatiquement
                    if (tx_en_attente >= tx_max_bloc) {
                        printf("\n[Info] Mempool plein, minage automatique...\n");

                        int idx_mineur = rand() % ma_liste_accounts.nb_accounts;
                        Account *mineur_acc = &ma_liste_accounts.accounts[idx_mineur];
                        printf("[Info] Mineur tire au sort : %s\n", mineur_acc->str);

                        Block *bloc = create_nouveau_bloc(ma_monnaie->bc);
                        if (bloc == NULL) { break; }
                        strncpy(bloc->minerName, mineur_acc->str, MAX_STRING);
                        strncpy(bloc->comment, "Bloc transactions manuelles", MAX_STRING);

                        //dépiler et finaliser les transactions
                        int nb = 0;
                        while (nb < tx_max_bloc) {
                            Transaction *tx_piochee = defiler_mempool();
                            if (tx_piochee == NULL) break;
                            bloc->transactions = inserer_en_tete(bloc->transactions, tx_piochee);
                            bloc->nbTx++;
                            nb++;
                        }
                        creer_tx_coinbase(ma_monnaie, bloc, mineur_acc->str);
                        mine_block(bloc, ma_monnaie->bc->difficulty);

                        if (!ajouter_bloc_blockchain(ma_monnaie->bc, bloc, ma_monnaie->bc->difficulty)) {
                            printf("[Erreur] Bloc rejeté.\n");
                            free(bloc);
                        } else {
                            printf("[Succes] Bloc #%d miné avec %d transactions.\n", bloc->index, bloc->nbTx);
                        }
                        tx_en_attente = 0;
                        tx_max_bloc=(rand() % (MAXTX - 1)) + 1; //pour le prochain bloc 
                    }

                } else {
                    printf("Probleme lors de la selection des UTXO.\n");
                }
                break;


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
                //netoyage de tout l'espace en memoire
                vider_liste_utxo();
                if (ma_monnaie != NULL) {
                    liberer_blockchain(ma_monnaie->bc);
                    free(ma_monnaie);
                }
                printf("\nFermeture du programme.\n");
                return 0;
        }

        //affichage de la masse monetaire en fin de chaque boucle
        if (ma_monnaie != NULL) {
            printf("\n------------------------------------------------\n");
            printf("Masse monetaire totale : %ld BTU\n", ma_monnaie->moneySupply);
            printf("------------------------------------------------\n");
        }

    } while (choix != 8);

    return 0;
}