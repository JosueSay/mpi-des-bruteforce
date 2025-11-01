/*
  impl2_seq.c — Implementación 2 (SECUENCIAL) = altA
  - Lee texto plano por stdin.
  - Cifra con key_true para generar el 'cipher' y luego busca la clave por fuerza bruta 0..key_true.
  - Mide tiempo y escribe SIEMPRE en: data/impl2/sec.csv
  - Encabezado fijo:
    implementation,key,p,repetition,time_seconds,iterations_done,found,finder_rank,timestamp,hostname,phrase,text
  - 'implementation' = altA, p=1, repetition=1
  - DES real si se compila con -DUSE_OPENSSL -lcrypto; si no, XOR fallback.

  NOTA (opcional, ya incluido más abajo y comentado):
  - Puedes imprimir la llave encontrada y una vista previa del texto descifrado
    descomentando el bloque marcado como "OPCIONAL".
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "../include/common.h"
#include "../include/core_utils.h"
#include "../include/core_crypto.h"

static void printUsage(const char *p)
{
  fprintf(stderr, "uso (seq): %s encrypt <key56> <out_bin> <csv_path> <hostname>\n", p);
  fprintf(stderr, "           %s decrypt \"<frase>\" <key_upper> <p> <csv_path> <hostname> <in_bin>\n", p);
}

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    printUsage(argv[0]);
    return 2;
  }
  const char *mode = argv[1];

  if (strcmp(mode, "encrypt") == 0)
  {
    if (argc < 6)
    {
      printUsage(argv[0]);
      return 2;
    }

    uint64_t key56 = strtoull(argv[2], NULL, 10);
    const char *out_bin = argv[3];
    const char *csv = argv[4];
    const char *host = argv[5];

    size_t plain_len = 0;
    unsigned char *plain = readAllStdin(&plain_len);
    if (!plain || plain_len == 0)
    {
      fprintf(stderr, "stdin vacio\n");
      free(plain);
      return 3;
    }

    struct timespec t0 = nowMono();
    unsigned char *cipher = NULL;
    size_t cipher_len = 0;
    if (encryptDesEcb(key56, plain, plain_len, &cipher, &cipher_len) != 0)
    {
      fprintf(stderr, "encrypt fallo\n");
      free(plain);
      return 4;
    }
    struct timespec t1 = nowMono();
    time_span_t dt = diffMono(t0, t1);

    if (writeFile(out_bin, cipher, cipher_len) != 0)
    {
      fprintf(stderr, "no pude escribir %s\n", out_bin);
    }

    FILE *fp = fopen(csv, "a+");
    if (fp)
    {
      ensureHeader(fp, CSV_HEADER);
      char ts[64];
      isoUtcNow(ts, sizeof ts);
      char *plain_csv = csvSanitize(plain, plain_len);
      fprintf(fp, "impl2,encrypt,%llu,0,1,%.9f,0,0,0,%s,%s,\"\",\"%s\",%s\n",
              (unsigned long long)key56, dt.secs, ts, host, plain_csv, out_bin);
      free(plain_csv);
      fclose(fp);
    }
    else
    {
      fprintf(stderr, "no pude abrir csv %s: %s\n", csv, strerror(errno));
    }

    free(plain);
    free(cipher);
    return 0;
  }

  if (strcmp(mode, "decrypt") == 0 || strcmp(mode, "brute") == 0)
  {
    if (argc < 8)
    {
      printUsage(argv[0]);
      return 2;
    }

    const char *phrase = argv[2];
    uint64_t key_upper = strtoull(argv[3], NULL, 10);
    int p = atoi(argv[4]);
    const char *csv = argv[5];
    const char *host = argv[6];
    const char *in_bin = argv[7];

    size_t cipher_len = 0;
    unsigned char *cipher = readFile(in_bin, &cipher_len);
    if (!cipher || cipher_len == 0)
    {
      fprintf(stderr, "no pude leer cipher %s\n", in_bin);
      free(cipher);
      return 3;
    }

    struct timespec t0 = nowMono();
    uint64_t iterations = 0;
    int found = 0;
    uint64_t found_key = UINT64_MAX;

    for (uint64_t k = 0; k <= key_upper; ++k)
    {
      unsigned char *dec = NULL;
      size_t dec_len = 0;
      if (decryptDesEcb(k, cipher, cipher_len, &dec, &dec_len) == 0)
      {
        iterations++;
        if (containsPhrase(dec, dec_len, phrase))
        {
          found = 1;
          found_key = k;

          /* --------------- OPCIONAL: imprimir llave y texto descifrado ---------------
             Descomenta este bloque si Wicho quiere ver la llave y un preview del texto.
             OJO: 'dec' puede contener bytes no imprimibles; por eso usamos un preview.

          // printf("[OK][SEQ] key = %llu (0x%llX)\n",
          //        (unsigned long long)found_key, (unsigned long long)found_key);

          // {
          //   size_t tocopy = dec_len < 256 ? dec_len : 255;  // límite de preview
          //   unsigned char preview[256];
          //   memcpy(preview, dec, tocopy);
          //   preview[tocopy] = '\0';
          //   printf("[OK][SEQ] plaintext_preview(len=%zu) = \"%s\"\n",
          //          dec_len, (char*)preview);
          // }
          --------------------------------------------------------------------------- */

          free(dec);
          break;
        }
      }
      free(dec);
    }
    struct timespec t1 = nowMono();
    time_span_t dt = diffMono(t0, t1);

    if (found)
      printf("found=%llu\n", (unsigned long long)found_key);
    else
      printf("no encontrado\n");

    FILE *fp = fopen(csv, "a+");
    if (fp)
    {
      ensureHeader(fp, CSV_HEADER);
      char ts[64];
      isoUtcNow(ts, sizeof ts);
      fprintf(fp,
              "impl2,decrypt,%llu,%d,1,%.9f,%llu,%d,%d,%s,%s,\"%s\",\"\",%s\n",
              (unsigned long long)key_upper,
              p, dt.secs,
              (unsigned long long)iterations,
              found, 0,
              ts, host, phrase, in_bin);
      fclose(fp);
    }
    else
    {
      fprintf(stderr, "no pude abrir csv %s: %s\n", csv, strerror(errno));
    }

    free(cipher);
    return found ? 0 : 1;
  }

  printUsage(argv[0]);
  return 2;
}
