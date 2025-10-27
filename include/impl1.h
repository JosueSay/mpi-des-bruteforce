#ifndef IMPL1_H
#define IMPL1_H

#include <stddef.h>
#include <stdint.h>

int encryptDesEcb(uint64_t key56, const unsigned char *in, size_t len,
                  unsigned char **out, size_t *out_len);
int decryptDesEcb(uint64_t key56, const unsigned char *in, size_t len,
                  unsigned char **out, size_t *out_len);

int containsPhrase(const unsigned char *buf, size_t len, const char *phrase);

#endif