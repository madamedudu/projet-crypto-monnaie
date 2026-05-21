#include "cryptographie.h"
#include "sha256_utils.h"
#include <stdlib.h>
#include "define.h"
#include "base58-code-decode.h"
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/bn.h>
#include <openssl/obj_mac.h>
#include <openssl/evp.h>
#include <openssl/ripemd.h>
#include <stdint.h>

unsigned char *SHA256(const unsigned char *d, size_t n, unsigned char *md);
#define SHA256_DIGEST_LENGTH 32

typedef unsigned char byte;

void hex_string2int_array(byte * hex_string,uint8_t tab[33]){
    char sub[3];
    int j = 0;

    for (int i = 0 ; i < 65 ; i = i + 2){
        strncpy(sub, (char *)hex_string + i, 2);
        sub[2] = '\0';
        tab[j] = (int)strtol(sub, NULL, 16);
        j++;
    }
}

void calc_address(char address[35],char * pub_key_char){
  unsigned char rmd[5 + RIPEMD160_DIGEST_LENGTH];
  uint8_t pub_key_int[33] = {0};
  hex_string2int_array((byte *)pub_key_char,pub_key_int);
  rmd[0] = 0; // adresse réseau

  RIPEMD160(SHA256((unsigned char *)pub_key_int, 33, 0),SHA256_DIGEST_LENGTH,rmd + 1);

memcpy(rmd + 21,SHA256(SHA256(rmd, 21, 0),SHA256_DIGEST_LENGTH,0),4);
  base58_encode(rmd, 25, address, 34);

  /* Count the number of 1s at the beginning of the address */
  int n = 0;
  for (n = 0; address[n] == '1'; n++);

  /* Do we need to remove any 1s? */
  if (n > 1) {
/* Move the memory so that the address begins at the final 1 */
      memmove(address, address + (n-1), 34-(n-1));

      /* Force the address to finish at the correct length */
      address[34-(n-1)] = '\0';
  }
    //printf("Address: %s\n\n", address);
}





void generer_cles(Account *compte){
    if(compte == NULL) return;

    //Creation de la clé ECDSA
    EC_KEY *paire_cles; //structure qui contient la clé privée et la clé publique
    paire_cles = EC_KEY_new_by_curve_name(NID_secp256k1); //On créer une paire ECDSA

    if(paire_cles == NULL){
        printf("Erreur creation paire de cles\n");
        return;
    }

    //génération des clés
    if(!EC_KEY_generate_key(paire_cles)){ //génère une clé privée aléatoire (c'est un très grand nombre)
        printf("Erreur generation cles\n");
        return;
    }





    //Récupération de la clé privée
    const BIGNUM *cle_privee;
    cle_privee = EC_KEY_get0_private_key(paire_cles); //récupère la clé privée qui a été générée

    //on convertit la clé privée en hexadécimal
    char *cle_privee_hex;
    cle_privee_hex = BN_bn2hex(cle_privee);

    //on enregistre la clé dans le compte
    strncpy((char*)compte->priv_key, cle_privee_hex, HASHLENGTH);





    //Récupération de la clé publique
    const EC_POINT *cle_publique;
    cle_publique = EC_KEY_get0_public_key(paire_cles);

    const EC_GROUP *groupe; // le groupe mathématique de la courbe
    groupe = EC_KEY_get0_group(paire_cles); //on récupère les paramètres de la courbe

    // on convertit la clé publique compressée en HEX
    char *cle_publique_hex;
    cle_publique_hex = EC_POINT_point2hex(groupe, cle_publique, POINT_CONVERSION_COMPRESSED, NULL);

    //on enregistre la clé publique dans le compte
    strncpy((char*)compte->pub_key, cle_publique_hex, 67);




    //Calcul de l'adresse bitcoin P2PKH
    calc_address((char*)compte->address, cle_publique_hex);




    //Libération de la mémoire
    OPENSSL_free(cle_privee_hex);
    OPENSSL_free(cle_publique_hex);
    EC_KEY_free(paire_cles);
}