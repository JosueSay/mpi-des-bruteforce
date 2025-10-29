#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/stat.h>

#include "../include/common.h"
#include "../include/core_utils.h"
#include "../include/core_crypto.h"

// uso
static void printUsage(const char *prog)
{
  fprintf(stderr, "uso:\n");
  fprintf(stderr, "  encrypt: echo \"<texto>\" | %s encrypt <key> <out_bin> <csv> <hostname>\n", prog);
  fprintf(stderr, "  decrypt: %s decrypt \"<frase>\" <key_upper> <p> <csv> <hostname> <in_bin>\n", prog);
  fprintf(stderr, "ejemplo encrypt:\n  echo \"Esta es una prueba\" | %s encrypt 123456 IO/outputs/cipher.bin data/impl1/sec.csv myhost\n", prog);
  fprintf(stderr, "ejemplo decrypt:\n  %s decrypt \"es una prueba\" 2000000 1 data/impl1/sec.csv myhost IO/outputs/cipher.bin\n", prog);
}

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    printUsage(argv[0]);
    return 2;
  }
  const char *mode = argv[1];

  // encrypt
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
              ts, hostname, plain_csv, out_bin);
      free(plain_csv);
      fclose(fp);
    }

    free(plain);
    free(cipher);
    return 0;
  }

  // decrypt
  if (strcmp(mode, "decrypt") == 0 || strcmp(mode, "brute") == 0)
  {
    if (argc < 8)
    {
      printUsage(argv[0]);
      return 2;
    }
    const char *frase = argv[2];
    uint64_t key_upper = strtoull(argv[3], NULL, 10);
    int p = atoi(argv[4]); // solo para registro en csv
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
      fprintf(fp,
              "impl1,decrypt,%llu,%d,1,%.9f,%llu,%d,%d,%s,%s,\"%s\",\"\",%s\n",
              (unsigned long long)key_upper,
              p,
              dt.secs,
              (unsigned long long)iterations_done,
              found,
              0,
              ts, hostname, frase, in_bin);
      fclose(fp);
    }

    free(cipher);
    return found ? 0 : 1;
  }

  printUsage(argv[0]);
  return 2;
}
