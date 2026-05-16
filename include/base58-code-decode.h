#ifndef BASE58_CODE_DECODE_H
#define BASE58_CODE_DECODE_H

#include <stdint.h>
typedef unsigned char byte;

int base58_decode(const char *base58, uint8_t *decoded, size_t *decoded_len);
char* base58_encode(byte *s, int s_size, char *out, int out_size);

#endif