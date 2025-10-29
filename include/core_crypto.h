#ifndef CORE_CRYPTO_H
#define CORE_CRYPTO_H

#include <stddef.h>
#include <stdint.h>

// cifrado/des cifrado des-ecb con openssl (key56 = 56 bits significativos)
int encryptDesEcb(uint64_t key56, const unsigned char *in, size_t len,
                  unsigned char **out, size_t *out_len);
int decryptDesEcb(uint64_t key56, const unsigned char *in, size_t len,
                  unsigned char **out, size_t *out_len);

// búsqueda de frase en buffer (asume texto ya depad-eado)
int containsPhrase(const unsigned char *buf, size_t len, const char *phrase);

#endif
