#ifndef CRYPTOGRAPHIE_H
#define CRYPTOGRAPHIE_H

#include "define.h"
#include <stdint.h>

typedef unsigned char byte;

void calc_address(char address[35], char *pub_key_char);
void generer_cles(Account *compte);
void signer_transaction(Transaction *transaction, BYTE *cle_privee, BYTE *signature);
void creer_lock_script(TxOutputs *out, char *signature, char *pubkey);
void creer_unlock_script(TxInputs *in,char * pubkey_hash);
void script_to_string(char **script, int size, char *out, int out_size);

#endif