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

//on essaye de voir si ça git pull ce main
int main() {
    printf("\n========== DEMARRAGE SYSTEME BLOCKCHAIN ==========\n");

    currency_t *ma_monnaie = NULL;
    ListeUsers ma_liste_users;
    ma_liste_users.nb_users = 0;
    int helicopter_deja_lance = 0; //faux pour l'instant
    int choix = 0;

    do {
        printf("\n================ MENU PRINCIPAL ================\n");
        printf("1. Creer la blockchain, la genesis, les utilisateurs\n");
        printf("2. Creer une transaction\n");
        printf("3. Verifier la coherence de la blockchain\n");
        printf("4. Sauver la blockchain au format json\n");
        printf("5. Lancer l'helicopter money (Distribution initiale)\n");
        printf("6. Afficher les utilisateurs\n");
        printf("7. Quitter\n");
        printf("================================================\n");
        
        int saisie_valide = 0;
        while (saisie_valide == 0) {
            printf("Votre choix (1-7) : ");
            
            //gestion de l'erreur si l'utilisateur tape une lettre
            if (scanf("%d", &choix) != 1) {
                while(getchar() != '\n'); 
                printf("[Erreur] Veuillez entrer un chiffre entier.\n");
            } 
            //gestion de l'erreur si le chiffre est hors limites (1 a 6)
            else if (choix < 1 || choix > 7) {
                printf("[Erreur] Veuillez entrer un chiffre entre 1 et 6.\n");
            } 
            else {
                saisie_valide = 1; 
            }
        }

        switch (choix) {
            
            case 1:
                printf("\n--- INITIALISATION ---\n");
                if (ma_monnaie == NULL) {
                    ma_monnaie = init_currency(); 
                    if (ma_monnaie != NULL) {
                        ma_liste_users = generer_users(MAX_USERS); 
                        
                        printf("[Succes] Blockchain, bloc Genesis et %d utilisateurs crees.\n\n", ma_liste_users.nb_users);
                        afficher_users(ma_liste_users);
                    } else {
                        printf("[Erreur] Echec de l'initialisation.\n");
                    }
                } else {
                    printf("[Info] La blockchain est deja initialisee.\n");
                }
                break;

            case 2:
                //transaction entre utilisateurs
                printf("\n--- NOUVELLE TRANSACTION ---\n");
                if (ma_monnaie == NULL || ma_liste_users.nb_users == 0) {
                    printf("[Erreur] Veuillez d'abord creer la blockchain et les utilisateurs (Option 1).\n");
                    break;
                }

                //affiche la liste des users
                printf("\n-- Utilisateurs enregistres --\n");
                for (int i = 0; i < ma_liste_users.nb_users; i++) {
                    printf("- %s (Solde : %ld BT)\n", ma_liste_users.users[i].adresse, (long)ma_liste_users.users[i].solde);
                }
                printf("------------------------------\n");

                char emetteur[MAX_STRING];
                char beneficiaire[MAX_STRING];
                long montant;

                //saisi des info trans
                printf("Entrez le nom de l'emetteur : ");
                scanf("%49s", emetteur);
                
                printf("Entrez le nom du beneficiaire : ");
                scanf("%49s", beneficiaire);
                
                printf("Entrez le montant : ");
                if (scanf("%ld", &montant) != 1) {
                    while(getchar() != '\n'); 
                    printf("[Erreur] Saisie du montant invalide. Annulation de la transaction.\n");
                    break;
                }

                //Verification si les emetteurs et benef existent ds le tab
                int idx_emetteur = -1;
                int idx_beneficiaire = -1;
                
                for (int i = 0; i < ma_liste_users.nb_users; i++) {
                    if (strcmp(emetteur, ma_liste_users.users[i].adresse) == 0) idx_emetteur = i;
                    if (strcmp(beneficiaire, ma_liste_users.users[i].adresse) == 0) idx_beneficiaire = i;
                }

                if (idx_emetteur == -1 || idx_beneficiaire == -1) {
                    printf("[Erreur] L'emetteur ou le beneficiaire n'existe pas. Annulation de la transaction.\n");
                    break;
                }

                if (montant <= 0 || ma_liste_users.users[idx_emetteur].solde < montant) {
                    printf("[Erreur] Solde insuffisant. Annulation de la transaction.\n");
                    break;
                }
                
                //solde des comptes maj
                ma_liste_users.users[idx_emetteur].solde -= montant;
                ma_liste_users.users[idx_beneficiaire].solde += montant;

                printf("\n[Succes] Transaction validee : %s a envoye %ld BT a %s.\n", emetteur, montant, beneficiaire);
                break;

            case 3:
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

                    export_blockchain_json(ma_liste_users, ma_monnaie->bc, nom_fichier);
                }
                break;

            case 5:
                printf("\n--- HELICOPTER MONEY ---\n");
                if (ma_monnaie == NULL || ma_liste_users.nb_users == 0) {
                    printf("[Erreur] Veuillez d'abord creer la blockchain (Option 1).\n");
                } 
                else if (helicopter_deja_lance == 1) {
                    printf("[Erreur] L'Helicopter Money a déjà été distribué.\n");
                } 
                else {
                    
                    run_helicopter_money(&ma_liste_users, ma_monnaie);
                    helicopter_deja_lance = 1; 
                    printf("[Succes] Distribution de l'helicopter money realisee.\n\n");
                    afficher_users(ma_liste_users);
                }
                break;
            
            case 6:
                printf("\n--- LISTE DES UTILISATEURS ---\n");
                if (ma_liste_users.nb_users == 0) {
                    printf("[Info] Aucun utilisateur n'est enregistré.\n");
                } else {
                    afficher_users(ma_liste_users);
                }
                break;

            case 7:
                printf("\nFermeture du programme.\n");
                return 0;
        }

        //affichage de la masse monetaire en fin de chaque boucle
        if (ma_monnaie != NULL) {
            printf("\n------------------------------------------------\n");
            printf("Masse monetaire totale : %ld BT\n", ma_monnaie->moneySupply);
            printf("------------------------------------------------\n");
        }

    } while (choix != 7);

    return 0;
}




void test_generer_users() {
    printf("=== TEST GENERER USERS ===\n");
    ListeUsers liste = generer_users(3);

    if (liste.nb_users != 3) {
        printf("ECHEC : mauvais nombre d'utilisateurs\n");
        return;
    }

    for (int i = 0; i < liste.nb_users; i++) {
        printf("User %d -> %s | Solde : %.2f\n",i + 1, liste.users[i].adresse, liste.users[i].solde);

        if (liste.users[i].solde != 0){
            printf("ECHEC : solde incorrect\n");
            return;
        }
    }

    printf("SUCCES : generer_users fonctionne\n");
    free(liste.users);
}

void test_export_json() {
    printf("=== TEST EXPORT JSON ===\n");
    ListeUsers liste = generer_users(2);

    currency_t *currency = init_currency();
    if (currency == NULL) {
        printf("ECHEC : init currency\n");
        return;
    }

    export_blockchain_json(liste, currency->bc, "test_blockchain.json");
    FILE *file = fopen("test_blockchain.json", "r");

    if (file == NULL) {
        printf("ECHEC : fichier non créé\n");
        return;
    }

    printf("SUCCES : fichier JSON créé\n");
    fclose(file);
    free(liste.users);
}

