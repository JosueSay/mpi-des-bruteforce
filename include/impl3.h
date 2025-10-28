#ifndef IMPL3_H
#define IMPL3_H

#include <stddef.h>
#include <stdint.h>

// Funciones comunes usadas en impl3 (igual que en impl1)
int encryptDesEcb(uint64_t key56,
                  const unsigned char *in, size_t len,
                  unsigned char **out, size_t *out_len);

int decryptDesEcb(uint64_t key56,
                  const unsigned char *in, size_t len,
                  unsigned char **out, size_t *out_len);

int containsPhrase(const unsigned char *buf, size_t len,
                   const char *phrase);

#endif
