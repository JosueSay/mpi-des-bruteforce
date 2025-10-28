#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "common.h"
#include "impl1.h"

// crea directorio si no existe
static void ensureDir(const char *path)
{
  struct stat st;
  if (stat(path, &st) == 0)
    return;
  mkdir(path, 0775);
}

// separa base y extensión simple
static void splitExt(const char *path, char *base, size_t nbase, char *ext, size_t next)
{
  const char *dot = strrchr(path, '.');
  if (!dot || dot == path)
  {
    snprintf(base, nbase, "%s", path);
    ext[0] = '\0';
  }
  else
  {
    size_t blen = (size_t)(dot - path);
    if (blen >= nbase)
      blen = nbase - 1;
    memcpy(base, path, blen);
    base[blen] = '\0';
    snprintf(ext, next, "%s", dot + 1);
  }
}

// comprueba existencia
static int pathExists(const char *path)
{
  struct stat st;
  return stat(path, &st) == 0;
}

// genera ruta única si ya existe
static void uniquePath(char *dst, size_t ndst, const char *suggested)
{
  if (!pathExists(suggested))
  {
    snprintf(dst, ndst, "%s", suggested);
    return;
  }
  char base[512], ext[128];
  splitExt(suggested, base, sizeof base, ext, sizeof ext);
  char ts[32];
  isoUtcNow(ts, sizeof ts);
  for (int i = 1; i < 10000; ++i)
  {
    if (ext[0] != '\0')
      snprintf(dst, ndst, "%s_%s_%d.%s", base, ts, i, ext);
    else
      snprintf(dst, ndst, "%s_%s_%d", base, ts, i);
    if (!pathExists(dst))
      return;
  }
  snprintf(dst, ndst, "%s", suggested);
}

// encripta archivo a salida, evitando sobrescribir
int impl1EncryptFile(uint64_t key56, const char *in_path, const char *out_path)
{
  unsigned char *in_buf = NULL, *cipher = NULL;
  size_t in_len = 0, cipher_len = 0;
  if (readEntireFile(in_path, &in_buf, &in_len) != 0)
    return -1;
  if (encryptDesEcb(key56, in_buf, in_len, &cipher, &cipher_len) != 0)
  {
    free(in_buf);
    return -2;
  }
  ensureDir("IO/outputs");
  char final_out[512];
  if (out_path && out_path[0])
    uniquePath(final_out, sizeof final_out, out_path);
  else
    uniquePath(final_out, sizeof final_out, "IO/outputs/cipher.bin");
  int rc = writeEntireFile(final_out, cipher, cipher_len);
  if (rc == 0)
    printf("archivo cifrado: %s | key=%llu\n", final_out, (unsigned long long)key56);
  free(cipher);
  free(in_buf);
  return rc == 0 ? 0 : -3;
}

// desencripta archivo a salida, evitando sobrescribir
int impl1DecryptFile(uint64_t key56, const char *in_path, const char *out_path)
{
  unsigned char *in_buf = NULL, *plain = NULL;
  size_t in_len = 0, plain_len = 0;
  if (readEntireFile(in_path, &in_buf, &in_len) != 0)
    return -1;
  if (decryptDesEcb(key56, in_buf, in_len, &plain, &plain_len) != 0)
  {
    free(in_buf);
    return -2;
  }
  ensureDir("IO/outputs");
  char final_out[512];
  if (out_path && out_path[0])
    uniquePath(final_out, sizeof final_out, out_path);
  else
    uniquePath(final_out, sizeof final_out, "IO/outputs/dec_output.txt");
  int rc = writeEntireFile(final_out, plain, plain_len);
  if (rc == 0)
    printf("archivo descifrado: %s | key=%llu\n", final_out, (unsigned long long)key56);
  free(plain);
  free(in_buf);
  return rc == 0 ? 0 : -3;
}

// logging CSV con header garantizado
int impl1LogCsv(FILE *csv,
                const char *implementation,
                run_mode_t mode,
                uint64_t key,
                int p,
                int repetition,
                double time_seconds,
                uint64_t iterations_done,
                int found,
                int finder_rank,
                const char *timestamp,
                const char *hostname,
                const char *phrase,
                const char *text,
                const char *in_path,
                const char *out_path)
{
  if (!csv)
    return -1;
  ensureHeader(csv, CSV_HEADER);
  fprintf(csv,
          "%s,%d,%llu,%d,%d,%.9f,%llu,%d,%d,%s,%s,\"%s\",\"%s\",\"%s\",\"%s\"\n",
          implementation ? implementation : "impl1",
          (int)mode,
          (unsigned long long)key,
          p,
          repetition,
          time_seconds,
          (unsigned long long)iterations_done,
          found,
          finder_rank,
          timestamp ? timestamp : "",
          hostname ? hostname : "",
          phrase ? phrase : "",
          text ? text : "",
          in_path ? in_path : "",
          out_path ? out_path : "");
  return 0;
}

// crack secuencial sobre archivo cifrado
int impl1CrackFileSeq(const char *cipher_path,
                      const char *phrase,
                      uint64_t start_key,
                      uint64_t end_key,
                      uint64_t *found_key,
                      uint64_t *iterations_done,
                      double *time_seconds)
{
  unsigned char *cipher = NULL;
  size_t cipher_len = 0;
  if (readEntireFile(cipher_path, &cipher, &cipher_len) != 0)
    return -1;

  struct timespec t0 = nowMono();
  uint64_t iters = 0;
  int ok = 0;
  uint64_t fkey = 0;

  if (end_key < start_key)
  {
    free(cipher);
    return -2;
  }

  for (uint64_t k = start_key; k <= end_key; ++k)
  {
    unsigned char *dec = NULL;
    size_t dec_len = 0;
    if (decryptDesEcb(k, cipher, cipher_len, &dec, &dec_len) == 0)
    {
      ++iters;
      if (containsPhrase(dec, dec_len, phrase))
      {
        ok = 1;
        fkey = k;
        free(dec);
        break;
      }
    }
    if (dec)
      free(dec);
    if (k == UINT64_MAX)
      break; // protección overflow
  }

  struct timespec t1 = nowMono();
  time_span_t dt = diffMono(t0, t1);

  if (found_key)
    *found_key = fkey;
  if (iterations_done)
    *iterations_done = iters;
  if (time_seconds)
    *time_seconds = dt.secs;

  free(cipher);
  return ok ? 0 : 1;
}

// parseo simple de flags estilo --k 42, --in file, --out file, --phrase "..."
static const char *findArg(int argc, char **argv, const char *flag)
{
  for (int i = 1; i < argc - 1; ++i)
    if (strcmp(argv[i], flag) == 0)
      return argv[i + 1];
  return NULL;
}

int main(int argc, char **argv)
{
  const char *mode = (argc >= 2) ? argv[1] : NULL;

  // encrypt
  if (mode && strcmp(mode, "--encrypt") == 0)
  {
    const char *k_str = findArg(argc, argv, "-k");
    const char *in_path = findArg(argc, argv, "-in");
    const char *out_path = findArg(argc, argv, "-out");
    if (!k_str || !in_path)
    {
      fprintf(stderr, "uso: %s --encrypt -k <key> -in <in.txt> [-out <cipher.bin>]\n", argv[0]);
      return 2;
    }
    uint64_t key = strtoull(k_str, NULL, 10);
    int rc = impl1EncryptFile(key, in_path, out_path);
    return rc == 0 ? 0 : 1;
  }

  // decrypt
  if (mode && strcmp(mode, "--decrypt") == 0)
  {
    const char *k_str = findArg(argc, argv, "-k");
    const char *in_path = findArg(argc, argv, "-in");
    const char *out_path = findArg(argc, argv, "-out");
    if (!k_str || !in_path)
    {
      fprintf(stderr, "uso: %s --decrypt -k <key> -in <cipher.bin> [-out <plain.txt>]\n", argv[0]);
      return 2;
    }
    uint64_t key = strtoull(k_str, NULL, 10);
    int rc = impl1DecryptFile(key, in_path, out_path);
    return rc == 0 ? 0 : 1;
  }

  // crack
  if (mode && strcmp(mode, "--crack") == 0)
  {
    const char *cipher_path = findArg(argc, argv, "-in");
    const char *phrase = findArg(argc, argv, "-phrase");
    const char *start_str = findArg(argc, argv, "-start");
    const char *end_str = findArg(argc, argv, "-end");
    const char *rep_str = findArg(argc, argv, "-rep");
    const char *csv_path = findArg(argc, argv, "-csv");
    const char *host_tag = findArg(argc, argv, "-host");

    if (!cipher_path || !phrase)
    {
      fprintf(stderr, "uso: %s --crack -in <cipher.bin> -phrase \"...\" [-start A] [-end B] [-rep N] [-csv path] [-host tag]\n", argv[0]);
      return 2;
    }

    uint64_t start_key = start_str ? strtoull(start_str, NULL, 10) : 0ULL;
    uint64_t end_key = end_str ? strtoull(end_str, NULL, 10) : ((1ULL << 56) - 1ULL);
    int repetition = rep_str ? atoi(rep_str) : 1;
    if (repetition < 1)
      repetition = 1;

    char host_buf[128] = {0};
    if (!host_tag)
    {
      gethostname(host_buf, sizeof host_buf - 1);
      host_tag = host_buf;
    }

    for (int r = 1; r <= repetition; ++r)
    {
      uint64_t fkey = 0, iters = 0;
      double secs = 0.0;
      int rc = impl1CrackFileSeq(cipher_path, phrase, start_key, end_key, &fkey, &iters, &secs);

      if (csv_path)
      {
        FILE *csv = fopen(csv_path, "a");
        if (!csv)
        {
          fprintf(stderr, "no pude abrir csv %s: %s\n", csv_path, strerror(errno));
        }
        else
        {
          char ts[32];
          isoUtcNow(ts, sizeof ts);
          impl1LogCsv(csv,
                      "impl1",
                      runModeCrackSeq,
                      fkey,
                      1,
                      r,
                      secs,
                      iters,
                      (rc == 0) ? 1 : 0,
                      0,
                      ts,
                      host_tag,
                      phrase,
                      "",
                      cipher_path,
                      "");
          fclose(csv);
        }
      }

      if (rc == 0)
        fprintf(stdout, "key=%llu encontrada; tiempo=%.6f s; iters=%llu\n",
                (unsigned long long)fkey, secs, (unsigned long long)iters);
      else
        fprintf(stdout, "no encontrada; tiempo=%.6f s; iters=%llu\n",
                secs, (unsigned long long)iters);
    }

    return 0;
  }

  fprintf(stderr, "modos: --encrypt | --decrypt | --crack\n");
  return 2;
}
