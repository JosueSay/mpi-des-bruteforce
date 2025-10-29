#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/stat.h>

#include "../include/common.h"
#include "../include/impl1.h"

#ifdef USE_OPENSSL
#include <openssl/des.h>
#endif

// lee todo stdin en buffer (malloc)
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

// malloc con exit si falla
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

// convierte string a versión segura para CSV (duplica " y reemplaza \r/\n por espacio)
static char *csvSanitize(const unsigned char *in, size_t len)
{
  size_t extra = 0;
  for (size_t i = 0; i < len; ++i)
    if (in[i] == '"')
      ++extra;
  char *out = (char *)xmalloc(len + extra + 1);
  size_t j = 0;
  for (size_t i = 0; i < len; ++i)
  {
    unsigned char c = in[i];
    if (c == '"')
    {
      out[j++] = '"';
      out[j++] = '"';
    }
    else if (c == '\n' || c == '\r')
    {
      out[j++] = ' ';
    }
    else
    {
      out[j++] = (char)c;
    }
  }
  out[j] = '\0';
  return out;
}

#ifdef USE_OPENSSL
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

// cifrado DES-ECB (o fallback xor)
int encryptDesEcb(uint64_t key56, const unsigned char *in, size_t len,
                  unsigned char **out, size_t *out_len)
{
#ifdef USE_OPENSSL
  size_t p_len;
  unsigned char *p = padBlock8(in, len, &p_len);
  *out = (unsigned char *)xmalloc(p_len);
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
#else
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

// descifrado DES-ECB (o fallback xor)
int decryptDesEcb(uint64_t key56, const unsigned char *in, size_t len,
                  unsigned char **out, size_t *out_len)
{
#ifdef USE_OPENSSL
  if (len % 8 != 0)
    return -1;
  unsigned char *tmp = (unsigned char *)xmalloc(len);

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
  unsigned char *unp;
  size_t unp_len;
  unp = unpadBlock8(tmp, len, &unp_len);
  free(tmp);
  *out = unp;
  *out_len = unp_len;
  return 0;
#else
  unsigned char *tmp = (unsigned char *)xmalloc(len);
  unsigned char b = (unsigned char)(key56 & 0xFFu);
  for (size_t i = 0; i < len; ++i)
    tmp[i] = in[i] ^ b;
  *out = tmp;
  *out_len = len;
  return 0;
#endif
}

// busca substring en buffer descifrado
int containsPhrase(const unsigned char *buf, size_t len, const char *phrase)
{
  if (!phrase || !*phrase)
    return 0;
  (void)len;
  const char *p = (const char *)buf;
  return strstr(p, phrase) != NULL;
}

// escribe buffer en archivo (binario)
static int writeFile(const char *path, const unsigned char *buf, size_t len)
{
  FILE *f = fopen(path, "wb");
  if (!f)
    return -1;
  size_t w = fwrite(buf, 1, len, f);
  fclose(f);
  return (w == len) ? 0 : -1;
}

// lee archivo binario completo (malloc)
static unsigned char *readFile(const char *path, size_t *out_len)
{
  FILE *f = fopen(path, "rb");
  if (!f)
    return NULL;
  fseek(f, 0, SEEK_END);
  long s = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (s <= 0)
  {
    fclose(f);
    *out_len = 0;
    return NULL;
  }
  unsigned char *buf = (unsigned char *)xmalloc((size_t)s);
  size_t r = fread(buf, 1, (size_t)s, f);
  fclose(f);
  if (r != (size_t)s)
  {
    free(buf);
    return NULL;
  }
  *out_len = (size_t)s;
  return buf;
}

static void printUsage(const char *prog)
{
  fprintf(stderr, "uso:\n");
  fprintf(stderr, "  encrypt: echo \"<texto>\" | %s encrypt <key> <out_bin> <csv> <hostname>\n", prog);
  fprintf(stderr, "  descencriptar: %s descencriptar \"<frase>\" <key_upper> <p> <csv> <hostname> <in_bin>\n", prog);
  fprintf(stderr, "ejemplo encrypt:\n  echo \"Esta es una prueba\" | %s encrypt 123456 cipher.bin data/impl1/sec.csv myhost\n", prog);
  fprintf(stderr, "ejemplo descencriptar:\n  %s descencriptar \"es una prueba\" 123456 1 data/impl1/sec.csv myhost cipher.bin\n", prog);
}

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    printUsage(argv[0]);
    return 2;
  }
  const char *mode = argv[1];

  // ---------- encrypt ----------
  if (strcmp(mode, "encrypt") == 0)
  {
    if (argc < 6)
    {
      printUsage(argv[0]);
      return 2;
    }
    uint64_t key_true = strtoull(argv[2], NULL, 10);
    const char *out_bin = argv[3];
    const char *csv_path = argv[4];
    const char *hostname = argv[5];

    struct stat st;
    if (stat(out_bin, &st) == 0)
    {
      fprintf(stderr, "out_bin ya existe: %s\n", out_bin);
      return 5;
    }

    size_t plain_len = 0;
    unsigned char *plain = readAllStdin(&plain_len);
    if (!plain || plain_len == 0)
    {
      fprintf(stderr, "stdin vacio\n");
      return 3;
    }

    struct timespec t0 = nowMono();
    unsigned char *cipher = NULL;
    size_t cipher_len = 0;
    if (encryptDesEcb(key_true, plain, plain_len, &cipher, &cipher_len) != 0)
    {
      fprintf(stderr, "error en encrypt\n");
      free(plain);
      return 4;
    }
    struct timespec t1 = nowMono();
    time_span_t dt = diffMono(t0, t1);

    if (writeFile(out_bin, cipher, cipher_len) != 0)
    {
      fprintf(stderr, "error escribiendo %s\n", out_bin);
      free(plain);
      free(cipher);
      return 6;
    }

    // CSV: registramos el texto plano (sanitizado); phrase vacío
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
      char *plain_csv = csvSanitize(plain, plain_len);
      fprintf(fp,
              "impl1,encrypt,%llu,0,1,%.9f,0,0,0,%s,%s,\"\",\"%s\",%s\n",
              (unsigned long long)key_true,
              dt.secs,
              ts,
              hostname,
              plain_csv,
              out_bin);
      free(plain_csv);
      fclose(fp);
    }

    free(plain);
    free(cipher);
    return 0;
  }

  // ---------- descencriptar (alias: brute para compat) ----------
  if (strcmp(mode, "descencriptar") == 0 || strcmp(mode, "brute") == 0)
  {
    if (argc < 8)
    {
      printUsage(argv[0]);
      return 2;
    }
    const char *frase = argv[2];
    uint64_t key_upper = strtoull(argv[3], NULL, 10);
    int p = atoi(argv[4]);
    const char *csv_path = argv[5];
    const char *hostname = argv[6];
    const char *in_bin = argv[7];

    size_t cipher_len = 0;
    unsigned char *cipher = readFile(in_bin, &cipher_len);
    if (!cipher || cipher_len == 0)
    {
      fprintf(stderr, "no pude leer cipher %s\n", in_bin);
      return 3;
    }

    struct timespec t0 = nowMono();
    uint64_t iterations_done = 0, found_key = UINT64_MAX;
    int found = 0;

    for (uint64_t k = 0; k <= key_upper; ++k)
    {
      unsigned char *dec = NULL;
      size_t dec_len = 0;
      if (decryptDesEcb(k, cipher, cipher_len, &dec, &dec_len) != 0)
      {
        free(dec);
        continue;
      }
      ++iterations_done;
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

    if (found)
      fprintf(stdout, "%llu encontrado; tiempo=%.6f s; iters=%llu\n",
              (unsigned long long)found_key, dt.secs, (unsigned long long)iterations_done);
    else
      fprintf(stdout, "no encontrado; tiempo=%.6f s; iters=%llu\n",
              dt.secs, (unsigned long long)iterations_done);

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
      // phrase con comillas, text vacío; mode siempre "descencriptar"
      fprintf(fp,
              "impl1,descencriptar,%llu,%d,1,%.9f,%llu,%d,%d,%s,%s,\"%s\",\"\",%s\n",
              (unsigned long long)key_upper,
              p,
              dt.secs,
              (unsigned long long)iterations_done,
              found,
              0,
              ts,
              hostname,
              frase,
              in_bin);
      fclose(fp);
    }

    free(cipher);
    return found ? 0 : 1;
  }

  printUsage(argv[0]);
  return 2;
}
