#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "../include/common.h"
#include "../include/impl1.h"

// define USE_OPENSSL y enlaza -lcrypto para usar DES real
#ifdef USE_OPENSSL
#include <openssl/des.h>
#endif

// header csv (secuencial/par comparten)
static const char *CSV_HEADER = "implementation,key,p,repetition,time_seconds,iterations_done,found,finder_rank,timestamp,hostname";

// leer stdin completo
static unsigned char *readAllStdin(size_t *out_len)
{
  size_t cap = 4096, len = 0;
  unsigned char *buf = (unsigned char *)malloc(cap);
  if (!buf)
    return NULL;
  for (;;)
  {
    size_t n = fread(buf + len, 1, cap - len, stdin);
    len += n;
    if (n == 0)
      break;
    if (len == cap)
    {
      cap *= 2;
      unsigned char *t = (unsigned char *)realloc(buf, cap);
      if (!t)
      {
        free(buf);
        return NULL;
      }
      buf = t;
    }
  }
  *out_len = len;
  return buf;
}

static void *xmalloc(size_t n)
{
  void *p = malloc(n);
  if (!p)
  {
    fprintf(stderr, "oom\n");
    exit(1);
  }
  return p;
}

// convierte key de 56 bits (uint64_t) a DES_cblock con paridad impar
#ifdef USE_OPENSSL
static void makeKeyFrom56(uint64_t key56, DES_cblock *k)
{
  memset(k, 0, sizeof(DES_cblock));
  for (int i = 0; i < 8; ++i)
  {
    uint8_t seven = (uint8_t)((key56 >> (56 - 7 * (i + 1))) & 0x7Fu);
    (*k)[i] = (unsigned char)(seven << 1); // deja bit de paridad en LSB
  }
  DES_set_odd_parity(k);
}
#endif

// padding pkcs7-like a múltiplos de 8
static unsigned char *padBlock8(const unsigned char *in, size_t len, size_t *out_len)
{
  size_t pad = 8 - (len % 8);
  if (pad == 0)
    pad = 8;
  *out_len = len + pad;
  unsigned char *out = (unsigned char *)xmalloc(*out_len);
  memcpy(out, in, len);
  memset(out + len, (int)pad, pad);
  return out;
}

static unsigned char *unpadBlock8(const unsigned char *in, size_t len, size_t *out_len)
{
  if (len == 0)
  {
    *out_len = 0;
    return NULL;
  }
  unsigned char pad = in[len - 1];
  size_t cut = (pad >= 1 && pad <= 8 && pad <= len) ? (len - pad) : len;
  unsigned char *out = (unsigned char *)xmalloc(cut ? cut : 1);
  if (cut)
    memcpy(out, in, cut);
  *out_len = cut;
  return out;
}

// cifrado
int encryptDesEcb(uint64_t key56, const unsigned char *in, size_t len,
                  unsigned char **out, size_t *out_len)
{
#ifdef USE_OPENSSL
  size_t p_len;
  unsigned char *p = padBlock8(in, len, &p_len);
  *out = (unsigned char *)xmalloc(p_len);
  *out_len = p_len;

  DES_cblock k;
  makeKeyFrom56(key56, &k);
  DES_key_schedule ks;
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
#else
  // fallback xor (solo testing)
  size_t p_len;
  unsigned char *p = padBlock8(in, len, &p_len);
  *out = (unsigned char *)xmalloc(p_len);
  *out_len = p_len;
  unsigned char b = (unsigned char)(key56 & 0xFFu);
  for (size_t i = 0; i < p_len; ++i)
    (*out)[i] = p[i] ^ b;
  free(p);
  return 0;
#endif
}

// descifrado
int decryptDesEcb(uint64_t key56, const unsigned char *in, size_t len,
                  unsigned char **out, size_t *out_len)
{
#ifdef USE_OPENSSL
  if (len % 8 != 0)
    return -1;
  unsigned char *tmp = (unsigned char *)xmalloc(len);

  DES_cblock k;
  makeKeyFrom56(key56, &k);
  DES_key_schedule ks;
  DES_set_key_unchecked(&k, &ks);

  for (size_t i = 0; i < len; i += 8)
  {
    DES_cblock ib, ob;
    memcpy(ib, in + i, 8);
    DES_ecb_encrypt(&ib, &ob, &ks, DES_DECRYPT);
    memcpy(tmp + i, ob, 8);
  }
  unsigned char *unp;
  size_t unp_len;
  unp = unpadBlock8(tmp, len, &unp_len);
  free(tmp);
  *out = unp;
  *out_len = unp_len;
  return 0;
#else
  // fallback xor simétrico
  unsigned char *tmp = (unsigned char *)xmalloc(len);
  unsigned char b = (unsigned char)(key56 & 0xFFu);
  for (size_t i = 0; i < len; ++i)
    tmp[i] = in[i] ^ b;
  *out = tmp;
  *out_len = len; // sin unpadding
  return 0;
#endif
}

// busca substring
int containsPhrase(const unsigned char *buf, size_t len, const char *phrase)
{
  if (!phrase || !*phrase)
    return 0;
  (void)len;
  const char *p = (const char *)buf;
  return strstr(p, phrase) != NULL;
}

static void printUsage(const char *prog)
{
  fprintf(stderr, "uso: echo \"<texto>\" | %s <frase> <key> <p> <ruta_csv> <hostname>\n", prog);
  fprintf(stderr, "ejem: echo \"Esta es una prueba\" | %s \"una prueba\" 123456 1 data/impl1/sec.csv myhost\n", prog);
}

int main(int argc, char **argv)
{
  if (argc < 6)
  {
    printUsage(argv[0]);
    return 2;
  }

  const char *frase = argv[1];
  uint64_t key_true = strtoull(argv[2], NULL, 10); // llave real y upper bound
  int p = atoi(argv[3]);                           // en seq será 1
  const char *csv_path = argv[4];
  const char *hostname = argv[5];

  // leer texto stdin
  size_t plain_len = 0;
  unsigned char *plain = readAllStdin(&plain_len);
  if (!plain || plain_len == 0)
  {
    fprintf(stderr, "stdin vacio\n");
    return 3;
  }

  // cifrar con la llave verdadera para obtener el cipher
  unsigned char *cipher = NULL;
  size_t cipher_len = 0;
  if (encryptDesEcb(key_true, plain, plain_len, &cipher, &cipher_len) != 0)
  {
    fprintf(stderr, "error en encrypt\n");
    free(plain);
    return 4;
  }

  // medir tiempo
  struct timespec t0 = nowMono();
  uint64_t iterations_done = 0;
  uint64_t found_key = UINT64_MAX;
  int found = 0;

  // búsqueda 0..key_true (incluyente)
  for (uint64_t k = 0; k <= key_true; ++k)
  {
    unsigned char *dec = NULL;
    size_t dec_len = 0;
    if (decryptDesEcb(k, cipher, cipher_len, &dec, &dec_len) != 0)
    {
      free(dec);
      continue;
    }
    ++iterations_done; // cuenta intento actual
    if (containsPhrase(dec, dec_len, frase))
    {
      found = 1;
      found_key = k;
      free(dec);
      break;
    }
    free(dec);
  }
  struct timespec t1 = nowMono();
  time_span_t dt = diffMono(t0, t1);

  // stdout info mínima
  if (found)
  {
    fprintf(stdout, "%llu encontrado; tiempo=%.6f s; iters=%llu\n",
            (unsigned long long)found_key, dt.secs, (unsigned long long)iterations_done);
  }
  else
  {
    fprintf(stdout, "no encontrado; tiempo=%.6f s; iters=%llu\n",
            dt.secs, (unsigned long long)iterations_done);
  }

  // csv append
  FILE *fp = fopen(csv_path, "a+");
  if (!fp)
  {
    fprintf(stderr, "no pude abrir csv %s: %s\n", csv_path, strerror(errno));
  }
  else
  {
    ensureHeader(fp, CSV_HEADER);
    char ts[32];
    isoUtcNow(ts, sizeof ts);
    // implementation,key,p,repetition,time_seconds,iterations_done,found,finder_rank,timestamp,hostname
    fprintf(fp, "impl1,%llu,%d,%d,%.9f,%llu,%d,%d,%s,%s\n",
            (unsigned long long)key_true,
            p,
            1,
            dt.secs,
            (unsigned long long)iterations_done,
            found,
            0, // finder_rank en secuencial
            ts,
            hostname);
    fclose(fp);
  }

  free(plain);
  free(cipher);
  return 0;
}
