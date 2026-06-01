#ifndef SCRIPT_H
#define SCRIPT_H

#include "define.h"

#define MAX_STACK 16
#define MAX_ITEM_LEN 132 // 128 pour les données + 4 pour les préfixes d'opcode

// Structure de la pile
typedef struct {
    char data[MAX_STACK][MAX_ITEM_LEN];
    int top; // Index du sommet de la pile (-1 si vide)
} Stack;

//fonctions de gestion de la pile
void stack_init(Stack *s);
int stack_push(Stack *s, const char *value);
int stack_pop(Stack *s, char *out);
int stack_peek(Stack *s, char *out);
int stack_is_empty(Stack *s);

// Énumération et Op-codes
typedef enum { OP_DUP, OP_EQ, OP_HASH, OP_VER } OpCode;
int op_dup(Stack *s); 
int op_eq(Stack *s); 
int op_hash(Stack *s); 
int op_ver(Stack *s, Transaction *tx); // Nécessite la structure Tx pour la vérification

#endif

