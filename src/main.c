#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "define.h"
#include "sha256_utils.h"
#include "blockchain.h"   
#include "bloc.h"

int main() {

    
//     //test d'ajout de bloc à la blockchain
//    printf("[1] Initialisation de la monnaie (Genesis)...\n");
//     currency_t *currency = init_currency();
//     if (currency == NULL) {
//         fprintf(stderr, "Erreur lors de l'initialisation de la monnaie.\n");
//         return EXIT_FAILURE;
//     } 

//     Blockchain *bc = currency->bc;
//     printf("Initialisation de la blockchain réussie avec le bloc Genesis.\n");



//     printf("[2] Création d'un nouveau bloc valide...\n");
//     Block *nouveau_bloc = malloc(sizeof(Block)); //nouveau bloc à ajouter
//     if (nouveau_bloc == NULL) {
//         fprintf(stderr, "Erreur lors de l'allocation du nouveau bloc.\n");
//         return EXIT_FAILURE;
//     }
//     Block *genesis = (Block *)bc->blocklist->info; // On récupère le bloc précédent (genesis)

//     // Remplissage du nouveau bloc
//     nouveau_bloc->index = genesis->index + 1;
//     strncpy((char *)nouveau_bloc->previousHash, (char *)genesis->blockHash, MAX_STRING);
//     nouveau_bloc->timestamp = time(NULL);
//     nouveau_bloc->nbTx = 0; // Pas de transactions pour ce test
//     nouveau_bloc->transactions = NULL; // Pas de transactions pour ce test
//     strncpy(nouveau_bloc->minerName, "Alice", MAX_STRING);
//     strncpy(nouveau_bloc->comment, "Premier bloc après le Genesis", MAX_STRING);
//     nouveau_bloc->nonce = 0; // Nonce à définir lors du minage


//     //init du merkle root à 0 pour le test (pas de transactions)
//     memset(nouveau_bloc->merkleTree, '0', HASHLENGTH - 1);
//     nouveau_bloc->merkleTree[HASHLENGTH - 1] = '\0';

//     // Mine le bloc pour remplir le hash
//     printf("Minage du nouveau bloc...\n");
//     mine_block(nouveau_bloc, bc->difficulty); 

//     // Ajout du bloc à la blockchain
//     printf("Ajout du nouveau bloc à la blockchain...\n");
//     if (ajouter_bloc_blockchain(bc, nouveau_bloc, bc->difficulty)) {
//         printf("SUCCES: Nouveau bloc ajouté à la blockchain avec succès !\n");
//     } else {
//         fprintf(stderr, "Erreur lors de l'ajout du nouveau bloc à la blockchain.\n");
//         free(nouveau_bloc);
//         return EXIT_FAILURE;
//     }

//     printf("[3] Vérification de la validité de la blockchain...\n");
//     if (verification_blockchain(bc)) {
//         printf("SUCCES: La blockchain est valide !\n");
//     } else {
//         fprintf(stderr, "Erreur : La blockchain est invalide.\n");
//         return EXIT_FAILURE;
//     }

//     printf("[4] Test de sécurité : Un pirate tente de modifier le bloc...\n");
    
//     // On crée un bloc pirate identique au précédent
//     Block *bloc_pirate = malloc(sizeof(Block));
//     if (bloc_pirate != NULL) {
//         memcpy(bloc_pirate, nouveau_bloc, sizeof(Block)); 
        
//         // Le pirate modifie une info (l'index) mais ne recalcule pas le minage !
//         bloc_pirate->index = 999; 
//         strncpy(bloc_pirate->comment, "Bloc hacké !", MAX_STRING);
        
//         printf("Passage du bloc hacké à la douane...\n");
//         if (ajouter_bloc_blockchain(bc, bloc_pirate, bc->difficulty)) {
//             printf("CATASTROPHE : La douane a laissé passer le pirate !\n");
//         } else {
//             printf("SÉCURITÉ VALIDÉE : La douane a bien bloqué le pirate !\n");
//         }
//         free(bloc_pirate);
//     }
//     printf("[5] Lancement de l'audit global de la blockchain complète...\n");
    
//     // On appelle tes deux fonctions d'audit global
//     int chaine_valide = verification_blockchain(bc);
//     int merkle_valide = verification_merkle_blockchain(bc);

//     if (chaine_valide == 1 && merkle_valide == 1) {
//         printf("AUDIT RÉUSSI : La blockchain entière (Genesis + %d bloc) est 100%% intègre !\n", bc->nbBlocks - 1);
//     } else {
//         fprintf(stderr, "AUDIT ÉCHOUÉ : Une corruption a été détectée dans la chaîne.\n");
//         return EXIT_FAILURE;
//     }

//     printf("Tous les tests sont passés avec succès. La blockchain est sécurisée et fonctionnelle.\n");
    return 0;
}