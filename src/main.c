#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "define.h"
#include "sha256_utils.h"
#include "blockchain.h"   
#include "bloc.h"
#include "transaction.h"
#include "utils.h" 
#include "utxo.h" 
#include "marche.h" 

//on essaye de voir si ça git pull ce main
int main() {
    printf("\n========== DEMARRAGE SYSTEME BLOCKCHAIN ==========\n");

    currency_t *ma_monnaie = NULL;
    ListeAccounts ma_liste_accounts;
    ma_liste_accounts.nb_accounts = 0;
    int helicopter_deja_lance = 0; //faux pour l'instant
    int choix = 0;
    srand(time(NULL));

    do {
        printf("\n================ MENU PRINCIPAL ================\n");
        printf("1. Creer la blockchain, la genesis, les utilisateurs\n");
        printf("2. Lancer l'helicopter money (Distribution initiale)\n");
        printf("3. Creer une transaction\n");
        printf("4. Sauver la blockchain au format json\n");
        printf("5. Verifier la coherence de la blockchain\n");
        printf("6. Afficher les utilisateurs\n");
        printf("7. consulter registre utxo\n");
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
                printf("[Erreur] Veuillez entrer un chiffre entre 1 et 6.\n");
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
                
                int nb_comptes = 0;
                printf("Combien d'utilisateurs voulez-vous generer ? (Max %d) : ", MAX_USERS);
                if (scanf("%d", &nb_comptes) != 1) {
                    while(getchar() != '\n'); // vide le buffer en cas d'erreur de saisie
                    printf("Erreur de saisie.\n");
                    break;
                }
                
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
                if (ma_monnaie == NULL || ma_liste_accounts.nb_accounts == 0) {
                    printf("[Erreur] Veuillez d'abord creer la blockchain et les utilisateurs (Option 1).\n");
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
                
                printf("Entrez le montant : ");
                if (scanf("%ld", &montant) != 1) {
                    while(getchar() != '\n'); 
                    printf("[Erreur] Saisie du montant invalide.\n");
                    break;
                }

                int resultat = create_transaction(&ma_liste_accounts, ma_monnaie->bc, emetteur, beneficiaire, montant); // <--- MODIFIÉ ICI

                if (resultat == 1) {
                    printf("\n[Succes] Transaction enregistree !\n");
                } else {
                    printf("\n[Erreur] La transaction a ete refusee.\n");
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
                
                printf("\n--- VERIFICATION DE LA BLOCKCHAIN ---\n");
                if (ma_monnaie == NULL) {
                    printf("[Erreur] Veuillez d'abord creer la blockchain (Option 1).\n");
                } else {
                    if (verification_blockchain(ma_monnaie->bc)) {
                        printf("[Succes] La blockchain est parfaitement coherente.\n");
                    } else {
                        printf("[Erreur] Corruption detectee dans la blockchain.\n");
                    }
                }
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
                printf("\nFermeture du programme.\n");
                return 0;
        }

        //affichage de la masse monetaire en fin de chaque boucle
        if (ma_monnaie != NULL) {
            printf("\n------------------------------------------------\n");
            printf("Masse monetaire totale : %ld BT\n", ma_monnaie->moneySupply);
            printf("------------------------------------------------\n");
        }

    } while (choix != 8);

    return 0;
}