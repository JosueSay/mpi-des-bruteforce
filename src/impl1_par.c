#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <mpi.h>

#include "common.h"
#include "impl1.h"

#define TAG_NOTIFY 9901

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

// crack paralelo con rango [start_key, end_key]
int impl1CrackFilePar(const char *cipher_path,
                      const char *phrase,
                      uint64_t start_key,
                      uint64_t end_key,
                      int world_size,
                      int world_rank,
                      uint64_t *found_key,
                      uint64_t *iterations_done_local,
                      int *found_rank,
                      double *time_seconds)
{
  unsigned char *cipher = NULL;
  size_t cipher_len = 0;

  if (world_rank == 0)
  {
    if (readEntireFile(cipher_path, &cipher, &cipher_len) != 0)
      return -1;
  }

  MPI_Bcast(&cipher_len, 1, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
  if (world_rank != 0)
    cipher = (unsigned char *)malloc(cipher_len);
  MPI_Bcast(cipher, (int)cipher_len, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

  uint64_t space = (end_key >= start_key) ? (end_key - start_key + 1ULL) : 0ULL;
  uint64_t base = (space / (uint64_t)world_size);
  uint64_t rem = (space % (uint64_t)world_size);
  uint64_t my_chunk = base + ((uint64_t)world_rank < rem ? 1ULL : 0ULL);
  uint64_t offset = base * (uint64_t)world_rank + (uint64_t)((world_rank < (int)rem) ? world_rank : rem);
  uint64_t my_start = start_key + offset;
  uint64_t my_end = (my_chunk == 0) ? 0ULL : (my_start + my_chunk - 1ULL);

  unsigned long long msg[2] = {~0ULL, ~0ULL};
  MPI_Request req;
  MPI_Status st;
  MPI_Irecv(msg, 2, MPI_UNSIGNED_LONG_LONG, MPI_ANY_SOURCE, TAG_NOTIFY, MPI_COMM_WORLD, &req);

  uint64_t iters = 0;
  int local_found = 0;
  uint64_t local_key = 0ULL;
  double t0 = MPI_Wtime();

  for (uint64_t k = my_start; (my_chunk > 0) && (k <= my_end); ++k)
  {
    int flag = 0;
    MPI_Test(&req, &flag, &st);
    if (flag)
    {
      local_found = 1;
      local_key = msg[0];
      break;
    }

    unsigned char *dec = NULL;
    size_t dec_len = 0;
    if (decryptDesEcb(k, cipher, cipher_len, &dec, &dec_len) == 0)
    {
      iters++;
      if (containsPhrase(dec, dec_len, phrase))
      {
        local_found = 1;
        local_key = k;
        unsigned long long notify[2] = {(unsigned long long)local_key, (unsigned long long)world_rank};
        for (int dst = 0; dst < world_size; ++dst)
          MPI_Send(notify, 2, MPI_UNSIGNED_LONG_LONG, dst, TAG_NOTIFY, MPI_COMM_WORLD);
        free(dec);
        break;
      }
    }
    if (dec)
      free(dec);
  }

  MPI_Wait(&req, &st);
  if (msg[0] != ~0ULL)
  {
    local_found = 1;
    local_key = msg[0];
  }

  double t1 = MPI_Wtime();

  unsigned long long g_key = (unsigned long long)local_key;
  int g_rank = local_found ? world_rank : -1;
  MPI_Allreduce(MPI_IN_PLACE, &g_key, 1, MPI_UNSIGNED_LONG_LONG, MPI_MAX, MPI_COMM_WORLD);
  MPI_Allreduce(MPI_IN_PLACE, &g_rank, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

  uint64_t total_iters = 0ULL;
  MPI_Reduce(&iters, &total_iters, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

  if (world_rank == 0)
  {
    if (found_key)
      *found_key = (uint64_t)g_key;
    if (found_rank)
      *found_rank = g_rank;
    if (time_seconds)
      *time_seconds = (t1 - t0);
  }
  if (iterations_done_local)
    *iterations_done_local = iters;

  if (cipher)
    free(cipher);
  return (g_rank >= 0) ? 0 : 1;
}

// parseo de flags estilo --k 42, --in file, --out file, --phrase "..."
static const char *findArg(int argc, char **argv, const char *flag)
{
  for (int i = 1; i < argc - 1; ++i)
    if (strcmp(argv[i], flag) == 0)
      return argv[i + 1];
  return NULL;
}

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);
  int rank = 0, world = 1;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &world);

  const char *mode = (argc >= 2) ? argv[1] : NULL;

  // modo encrypt
  if (mode && strcmp(mode, "--encrypt") == 0)
  {
    if (rank != 0)
    {
      MPI_Finalize();
      return 0;
    }
    const char *k_str = findArg(argc, argv, "-k");
    const char *in_path = findArg(argc, argv, "-in");
    const char *out_path = findArg(argc, argv, "-out");
    if (!k_str || !in_path)
    {
      fprintf(stderr, "uso: %s --encrypt -k <key> -in <in.txt> [-out <cipher.bin>]\n", argv[0]);
      MPI_Finalize();
      return 2;
    }
    uint64_t key = strtoull(k_str, NULL, 10);
    int rc = impl1EncryptFile(key, in_path, out_path);
    MPI_Finalize();
    return rc == 0 ? 0 : 1;
  }

  // modo decrypt
  if (mode && strcmp(mode, "--decrypt") == 0)
  {
    if (rank != 0)
    {
      MPI_Finalize();
      return 0;
    }
    const char *k_str = findArg(argc, argv, "-k");
    const char *in_path = findArg(argc, argv, "-in");
    const char *out_path = findArg(argc, argv, "-out");
    if (!k_str || !in_path)
    {
      fprintf(stderr, "uso: %s --decrypt -k <key> -in <cipher.bin> [-out <plain.txt>]\n", argv[0]);
      MPI_Finalize();
      return 2;
    }
    uint64_t key = strtoull(k_str, NULL, 10);
    int rc = impl1DecryptFile(key, in_path, out_path);
    MPI_Finalize();
    return rc == 0 ? 0 : 1;
  }

  // modo crack
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
      if (rank == 0)
        fprintf(stderr, "uso: %s --crack -in <cipher.bin> -phrase \"...\" [-start A] [-end B] [-rep N] [-csv path] [-host tag]\n", argv[0]);
      MPI_Finalize();
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
      uint64_t fkey = 0, iters_local = 0;
      int f_rank = -1;
      double secs = 0.0;
      int rc = impl1CrackFilePar(cipher_path, phrase, start_key, end_key, world, rank, &fkey, &iters_local, &f_rank, &secs);

      if (rank == 0 && csv_path)
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
          uint64_t sum_iters = 0ULL;
          MPI_Reduce(&iters_local, &sum_iters, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
          impl1LogCsv(csv,
                      "impl1",
                      runModeCrackPar,
                      fkey, // key encontrada (0 si no)
                      world,
                      r,
                      secs,
                      sum_iters,
                      (rc == 0) ? 1 : 0,
                      f_rank,
                      ts,
                      host_tag,
                      phrase,
                      "",
                      cipher_path,
                      "");
          fclose(csv);
        }
      }
      else
      {
        // rank != 0 aún debe participar del reduce para iters
        uint64_t sum_dummy = 0ULL;
        MPI_Reduce(&iters_local, &sum_dummy, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
      }
      MPI_Barrier(MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
  }

  if (rank == 0)
    fprintf(stderr, "modos: --encrypt | --decrypt | --crack\n");
  MPI_Finalize();
  return 2;
}
