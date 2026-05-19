#include <stdio.h>
#include <string.h>
#include "script.h"

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

// Dépile une valeur (Retire et récupère l'élément au sommet)
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

// Regarde l'élément au sommet sans le retirer
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