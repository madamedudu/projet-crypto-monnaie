#include <stdio.h>
#include <string.h>
#include "script.h"
#include "cryptographie.h"
// Initialise la pile en placant le curseur a -1
void stack_init(Stack *s) {
    if (s != NULL) {
        s->top = -1;
    }
}

// Vérifie si la pile est vide
int stack_is_empty(Stack *s) {
    if (s == NULL) return 1;
    return (s->top == -1);
}

// Empile une valeur
// Retourne 1 si succès, 0 en cas d'overflow
int stack_push(Stack *s, const char *value) {
    if (s == NULL || value == NULL) return 0;

    // Vérification de la taille max de la pile (Overflow)
    if (s->top >= MAX_STACK - 1) {
        printf("[Stack Error] Overflow ! Impossible d'empiler, la pile est pleine.\n");
        return 0;
    }

    s->top++;
    // Copie dans le tableau fixe sans allocation dynamique
    strncpy(s->data[s->top], value, MAX_ITEM_LEN - 1);
    s->data[s->top][MAX_ITEM_LEN - 1] = '\0';

    return 1;
}

// Depile une valeur (Retire et récupère l'élément au sommet)
// Retourne 1 si succès, -1 si la pile est vide
int stack_pop(Stack *s, char *out) {
    if (s == NULL || stack_is_empty(s)) {
        return -1; 
    }

    if (out != NULL) {
        strcpy(out, s->data[s->top]);
    }

    s->top--;
    return 1;
}

// Regarde l'element au sommet sans le retirer
// Retourne 1 si succès, -1 si la pile est vide
int stack_peek(Stack *s, char *out) {
    if (s == NULL || stack_is_empty(s)) {
        return -1;
    }
    if (out != NULL) {
        strcpy(out, s->data[s->top]);
    }

    return 1;
}

// DUP : Duplique l'element au sommet de la pile
int op_dup(Stack *s) { 
    char sommet[MAX_ITEM_LEN];
    
    // On regarde le sommet sans le retirer, si erreur on renvoie 0
    if (stack_peek(s, sommet) != 1){
        return 0;
    }
    // On re-empile le sommet trouve
    return stack_push(s, sommet);
}

// EQ : Depile deux éléments, les compare, et pousse "TRUE" ou "FALSE"
int op_eq(Stack *s) {
    char item1[MAX_ITEM_LEN];
    char item2[MAX_ITEM_LEN];
    
    if (stack_pop(s, item1) != 1) {
        return 0;
    }
    if (stack_pop(s, item2) != 1) {
        return 0;
    }

    if (strcmp(item1, item2) == 0) {
        return stack_push(s, "TRUE");
    } else {
        return stack_push(s, "FALSE");
    }
}

// HASH : Remplace l'élément au sommet par son empreinte SHA256
int op_hash(Stack *s) { 
    char cible[MAX_ITEM_LEN];
    char resultat_hash[65]; 
    
    if (stack_pop(s, cible) != 1) {
        return 0;
    }
    sha256ofString((BYTE *)cible, resultat_hash); 
    
    return stack_push(s, resultat_hash);
}

// VER : Dépile la clé publique, puis la signature, et valide via ECDSA
int op_ver(Stack *s, Transaction *tx) {
    char pub_key_hex[MAX_ITEM_LEN];
    char signature_hex[MAX_ITEM_LEN];
    
    if (stack_pop(s, pub_key_hex) != 1) {
        return 0;
    }
    if (stack_pop(s, signature_hex) != 1) {
        return 0;
    }

    if (verifier_transaction_ecdsa(tx, (BYTE *)pub_key_hex, signature_hex) == 1) { 
        return stack_push(s, "TRUE");
    } else {
        return stack_push(s, "FALSE");
    }
}

//fonction booleene qui verifie que la pile est TRUE
int execute_script(char **lock, int lock_size, char **unlock, int unlock_size,Transaction *tx){
    Stack s;
    stack_init(&s);
    //on est dans le cas du lock script, on empile les éléments du lock script
    for (int i = 0; i < lock_size; i++) {
        if (lock[i] == NULL) continue;

        if (strcmp(lock[i], "DUP") == 0) {
            if (!op_dup(&s)) return 0;
        } 
        else if (strcmp(lock[i], "HASH") == 0) {
            if (!op_hash(&s)) return 0;
        } 
        else if (strcmp(lock[i], "EQ") == 0) {
            if (!op_eq(&s)) return 0; 
        } 
        else if (strcmp(lock[i], "VER") == 0) {
            if (!op_ver(&s, tx)) return 0;
        } 
        else {
            if (!stack_push(&s, lock[i])) return 0;
        }
    }

    //on est dans le cas du unlock script, on empile les éléments du unlock script
    for (int i = 0; i < unlock_size; i++) {
        if (unlock[i] == NULL) continue;

        if (strcmp(unlock[i], "DUP") == 0) {
            if (!op_dup(&s)) return 0;
        } 
        else if (strcmp(unlock[i], "HASH") == 0) {
            if (!op_hash(&s)) return 0;
        } 
        else if (strcmp(unlock[i], "EQ") == 0) {
            if (!op_eq(&s)) return 0;
        } 
        else if (strcmp(unlock[i], "VER") == 0) {
            if (!op_ver(&s, tx)) return 0;
        } 
        else {
            if (!stack_push(&s, unlock[i])) return 0;
        }
    }

    /*
    Verification finale de la file et fonction op_ver a empilé true ça veut dire qu'on 
    doit retrouver true au sommet de la pile
    */
    char resultat_final[MAX_ITEM_LEN];
    if (stack_peek(&s, resultat_final) == 1) {
        if (strcmp(resultat_final, "TRUE") == 0) {
            return 1; // Le script est valide
        }

    }
    return 0; // Le script est invalide   

}