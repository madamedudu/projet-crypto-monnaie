#ifndef CRYPTOGRAPHIE_H
#define CRYPTOGRAPHIE_H

#include "define.h"

void generer_cles(Account * compte );
void signer_transaction(Transaction *transaction, BYTE *cle_privee, BYTE *signature);
int verifier_signature(Transaction *transaction, BYTE *cle_privee, BYTE *signature);
void creer_lock_script(TxOutputs *out, BYTE *cle_publique);
void creer_unlock_script(TxInputs *input, BYTE *signature, BYTE *cle_publique);
int executer_script(TxInputs* input, TxOutputs* output, Transaction* transaction);

#endif