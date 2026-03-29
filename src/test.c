#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "define.h"
#include "blockchain.h"
#include "bloc.h"
#include "transaction.h"
#include "utils.h"

// --- 1. FONCTION D'AUDIT INTÉGRÉE ---

void audit_global_blockchain(Blockchain *bc) {
    printf("\n--- LANCEMENT DE L'AUDIT DE SÉCURITÉ ---\n");

    // Appel de tes fonctions de vérification situées dans blockchain.c
    int chaine_ok = verification_blockchain(bc);
    int merkle_ok = verification_merkle_blockchain(bc);

    if (chaine_ok && merkle_ok) {
        printf("RESULTAT : La blockchain est [VALIDE]. L'intégrité est garantie.\n");
    } else {
        printf("ATTENTION : La blockchain est [CORROMPUE] ! Fraude détectée.\n");
    }
    printf("----------------------------------------\n");
}

// --- 2. FONCTIONS UTILITAIRES POUR LES TESTS ---

void ajouter_tx_a_liste(Slist **liste, Transaction *tx) {
    Slist *nouveau = malloc(sizeof(Slist));
    nouveau->info = (void *)tx;
    nouveau->next = *liste;
    *liste = nouveau;
}

// --- 3. SCÉNARIOS DE TESTS ---

void test_utilisateurs_et_helicopter_money() {
    printf("\n[TEST 1] Initialisation et Helicopter Money...\n");
    
    currency_t *ma_monnaie = init_currency(); 
    assert(ma_monnaie != NULL);

    // Test de génération des utilisateurs (Tâche 4.1)
    ListeUsers ma_liste = generer_users(MAX_USERS); 
    assert(ma_liste.nb_users == MAX_USERS);
    assert(ma_liste.users[0].solde == 0);

    // Test Helicopter Money (Tâche 4.2)
    run_helicopter_money(&ma_liste, ma_monnaie);
    
    // Vérification des soldes
    assert(ma_liste.users[0].solde == HELIREWARD);
    assert(ma_monnaie->moneySupply == (long)MAX_USERS * HELIREWARD);

    printf("=> SUCCÈS : Utilisateurs et Masse Monétaire OK.\n");
}

void test_creation_bloc_et_merkle() {
    printf("\n[TEST 2] Création de Bloc, Merkle et Minage (Tâche 4.4)...\n");
    
    currency_t *mon_syst = init_currency();
    Blockchain *bc = mon_syst->bc;

    Block *nouveau = malloc(sizeof(Block));
    nouveau->index = 1;
    nouveau->timestamp = time(NULL);
    nouveau->nonce = 0;
    nouveau->transactions = NULL;
    nouveau->nbTx = 0;

    Block *genesis = (Block *)bc->blocklist->info;
    strncpy((char*)nouveau->previousHash, (char*)genesis->blockHash, HASHLENGTH);

    // Ajout d'une transaction pour tester Merkle
    Transaction *t1 = malloc(sizeof(Transaction));
    strncpy((char*)t1->adSender, "USER_1", HASHLENGTH);
    strncpy((char*)t1->adReceiver, "USER_2", HASHLENGTH);
    t1->txAmount = 50;
    t1->timestamp = time(NULL);

    ajouter_tx_a_liste(&(nouveau->transactions), t1);
    nouveau->nbTx = 1;

    // Calcul Merkle et Minage
    merkle_root(nouveau->transactions, nouveau->nbTx, (char*)nouveau->merkleTree);
    mine_block(nouveau, bc->difficulty); 

    // Ajout et vérification
    assert(ajouter_bloc_blockchain(bc, nouveau, bc->difficulty) == 1);
    assert(bc->nbBlocks == 2);

    printf("=> SUCCÈS : Bloc et Merkle Root validés.\n");
}

void test_audit_complet_et_corruption() {
    printf("\n[TEST 3] Audit Global et Test de Corruption (Tâche 4.5)...\n");

    currency_t *mon_syst = init_currency();
    
    // Audit sur une chaîne saine
    audit_global_blockchain(mon_syst->bc); 

    // Simulation d'une attaque
    printf("Attaque : Modification d'une transaction dans le Genesis...\n");
    Block *genesis = (Block *)mon_syst->bc->blocklist->info;
    
    // On change le merkleTree manuellement pour simuler une altération
    genesis->merkleTree[5] = 'X'; 

    // L'audit doit maintenant détecter la corruption
    audit_global_blockchain(mon_syst->bc);
    
    // Test unitaire sur la fonction de hash
    assert(verification_hash_bloc(genesis) == 0);
    printf("=> SÉCURITÉ : La corruption a bien été détectée par l'audit.\n");
}

int main() {
    printf("========== DÉMARRAGE DE LA SUITE DE TESTS TECHNIQUES ==========\n");

    test_utilisateurs_et_helicopter_money();
    test_creation_bloc_et_merkle();
    test_audit_complet_et_corruption();

    printf("\n==========      TOUS LES TESTS SONT AU VERT        ==========\n");
    return 0;
}