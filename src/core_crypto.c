#include "../include/core_crypto.h"
#include <openssl/des.h>
#include <stdlib.h>
#include <string.h>

// genera clave des desde 56 bits (paridad impar en cada byte)
static void makeKeyFrom56(uint64_t key56, DES_cblock *k)
{
  memset(k, 0, sizeof(DES_cblock));
  for (int i = 0; i < 8; ++i)
  {
    uint8_t seven = (uint8_t)((key56 >> (56 - 7 * (i + 1))) & 0x7Fu);
    (*k)[i] = (unsigned char)(seven << 1);
  }
  DES_set_odd_parity(k);
}

// pkcs7-like a múltiplos de 8
static unsigned char *padBlock8(const unsigned char *in, size_t len, size_t *out_len)
{
  size_t pad = 8 - (len % 8);
  if (pad == 0)
    pad = 8;
  *out_len = len + pad;
  unsigned char *out = (unsigned char *)malloc(*out_len);
  if (!out)
    return NULL;
  memcpy(out, in, len);
  memset(out + len, (int)pad, pad);
  return out;
}

// remueve padding, devuelve copia tal cual
static unsigned char *unpadBlock8(const unsigned char *in, size_t len, size_t *out_len)
{
  if (len == 0)
  {
    *out_len = 0;
    return NULL;
  }
  unsigned char pad = in[len - 1];
  size_t cut = (pad >= 1 && pad <= 8 && pad <= len) ? (len - pad) : len;
  unsigned char *out = (unsigned char *)malloc(cut ? cut : 1);
  if (!out)
    return NULL;
  if (cut)
    memcpy(out, in, cut);
  *out_len = cut;
  return out;
}

// cifra des-ecb usando openssl
int encryptDesEcb(uint64_t key56, const unsigned char *in, size_t len,
                  unsigned char **out, size_t *out_len)
{
  size_t p_len = 0;
  unsigned char *p = padBlock8(in, len, &p_len);
  if (!p)
    return -1;
  *out = (unsigned char *)malloc(p_len);
  if (!*out)
  {
    free(p);
    return -1;
  }
  *out_len = p_len;

  DES_cblock k;
  DES_key_schedule ks;
  makeKeyFrom56(key56, &k);
  DES_set_key_unchecked(&k, &ks);

  for (size_t i = 0; i < p_len; i += 8)
  {
    DES_cblock ib, ob;
    memcpy(ib, p + i, 8);
    DES_ecb_encrypt(&ib, &ob, &ks, DES_ENCRYPT);
    memcpy(*out + i, ob, 8);
  }
  free(p);
  return 0;
}

// descifra des-ecb y quita padding
int decryptDesEcb(uint64_t key56, const unsigned char *in, size_t len,
                  unsigned char **out, size_t *out_len)
{
  if (len % 8 != 0)
    return -1;
  unsigned char *tmp = (unsigned char *)malloc(len);
  if (!tmp)
    return -1;

  DES_cblock k;
  DES_key_schedule ks;
  makeKeyFrom56(key56, &k);
  DES_set_key_unchecked(&k, &ks);

  for (size_t i = 0; i < len; i += 8)
  {
    DES_cblock ib, ob;
    memcpy(ib, in + i, 8);
    DES_ecb_encrypt(&ib, &ob, &ks, DES_DECRYPT);
    memcpy(tmp + i, ob, 8);
  }

  unsigned char *unp = NULL;
  size_t unp_len = 0;
  unp = unpadBlock8(tmp, len, &unp_len);
  free(tmp);
  if (!unp && unp_len == 0)
  {
    *out = NULL;
    *out_len = 0;
    return -1;
  }
  *out = unp;
  *out_len = unp_len;
  return 0;
}

// búsqueda naive de substring; convierte a cadena segura
int containsPhrase(const unsigned char *buf, size_t len, const char *phrase)
{
  if (!buf || !phrase || !*phrase)
    return 0;
  char *tmp = (char *)malloc(len + 1);
  if (!tmp)
    return 0;
  memcpy(tmp, buf, len);
  tmp[len] = '\0';
  int ok = strstr(tmp, phrase) != NULL;
  free(tmp);
  return ok;
}
