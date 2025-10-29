#ifndef IMPL1_H
#define IMPL1_H

#include <stddef.h>
#include <stdint.h>

// cifra en DES-ECB (key56 = 56 bits en uint64_t)
int encryptDesEcb(uint64_t key56, const unsigned char *in, size_t len,
                  unsigned char **out, size_t *out_len);

// descifra en DES-ECB
int decryptDesEcb(uint64_t key56, const unsigned char *in, size_t len,
                  unsigned char **out, size_t *out_len);

// busca substring en buffer descifrado
int containsPhrase(const unsigned char *buf, size_t len, const char *phrase);

#endif // IMPL1_H
