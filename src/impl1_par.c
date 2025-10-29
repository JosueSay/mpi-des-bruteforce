#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <mpi.h>

#include "../include/common.h"
#include "../include/core_utils.h"
#include "../include/core_crypto.h"

// imprime uso rápido
static void printUsage(const char *prog)
{
  fprintf(stderr, "uso:\n");
  fprintf(stderr, "  encrypt: echo \"<texto>\" | mpirun -np N %s encrypt <key> <out_bin> <csv> <hostname>\n", prog);
  fprintf(stderr, "  decrypt: mpirun -np N %s decrypt \"<frase>\" <key_upper> <p> <csv> <hostname> <in_bin>\n", prog);
  fprintf(stderr, "ejemplo encrypt:\n  echo \"Esta es una prueba\" | mpirun -np 1 %s encrypt 123456 cipher.bin data/impl1/par.csv myhost\n", prog);
  fprintf(stderr, "ejemplo decrypt:\n  mpirun -np 4 %s decrypt \"es una prueba de\" 2000000 4 data/impl1/par.csv myhost cipher.bin\n", prog);
}

// escribe fila csv estándar
static void writeCsvRow(const char *csv_path,
                        const char *implementation,
                        const char *mode,
                        unsigned long long key,
                        int p,
                        double secs,
                        unsigned long long iterations_done,
                        int found,
                        int finder_rank,
                        const char *timestamp,
                        const char *hostname,
                        const char *phrase,
                        const char *text,
                        const char *out_bin)
{
  FILE *fp = fopen(csv_path, "a+");
  if (!fp)
  {
    fprintf(stderr, "no pude abrir csv %s: %s\n", csv_path, strerror(errno));
    return;
  }
  ensureHeader(fp, CSV_HEADER);
  fprintf(fp,
          "%s,%s,%llu,%d,1,%.9f,%llu,%d,%d,%s,%s,\"%s\",\"%s\",%s\n",
          implementation, mode, key, p, secs, iterations_done, found, finder_rank,
          timestamp, hostname, phrase ? phrase : "", text ? text : "", out_bin ? out_bin : "");
  fclose(fp);
}

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);

  int world_size = 1, world_rank = 0;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

  if (argc < 2)
  {
    if (world_rank == 0)
      printUsage(argv[0]);
    MPI_Finalize();
    return 2;
  }

  const char *mode = argv[1];

  // encrypt
  if (strcmp(mode, "encrypt") == 0)
  {
    if (argc < 6)
    {
      if (world_rank == 0)
        printUsage(argv[0]);
      MPI_Finalize();
      return 2;
    }
    uint64_t key_true = strtoull(argv[2], NULL, 10);
    const char *out_bin = argv[3];
    const char *csv_path = argv[4];
    const char *hostname = argv[5];

    if (world_rank == 0)
    {
      size_t plain_len = 0;
      unsigned char *plain = readAllStdin(&plain_len);
      if (!plain)
      {
        fprintf(stderr, "no pude leer stdin\n");
        MPI_Finalize();
        return 3;
      }

      unsigned char *cipher = NULL;
      size_t cipher_len = 0;
      struct timespec t0 = nowMono();
      if (encryptDesEcb(key_true, plain, plain_len, &cipher, &cipher_len) != 0)
      {
        fprintf(stderr, "encrypt falló\n");
        free(plain);
        MPI_Finalize();
        return 4;
      }
      struct timespec t1 = nowMono();
      time_span_t dt = diffMono(t0, t1);

      if (writeFile(out_bin, cipher, cipher_len) != 0)
      {
        fprintf(stderr, "no pude escribir %s\n", out_bin);
      }

      char ts[32];
      isoUtcNow(ts, sizeof ts);
      char *text_csv = csvSanitize(plain, plain_len);
      writeCsvRow(csv_path, "impl1", "encrypt",
                  (unsigned long long)key_true,
                  world_size, dt.secs, 0ULL, 0, 0, ts, hostname,
                  "", text_csv, out_bin);
      free(text_csv);
      free(cipher);
      free(plain);
      fprintf(stdout, "ok; tiempo=%.6f s; out=%s\n", dt.secs, out_bin);
    }

    MPI_Finalize();
    return 0;
  }

  // decrypt
  if (strcmp(mode, "decrypt") == 0 || strcmp(mode, "brute") == 0)
  {
    if (argc < 8)
    {
      if (world_rank == 0)
        printUsage(argv[0]);
      MPI_Finalize();
      return 2;
    }

    const char *frase = argv[2];
    uint64_t key_upper = strtoull(argv[3], NULL, 10);
    int p_param = atoi(argv[4]); // scripts pasan p=np para registro
    const char *csv_path = argv[5];
    const char *hostname = argv[6];
    const char *in_bin = argv[7];

    size_t cipher_len = 0;
    unsigned char *cipher = readFile(in_bin, &cipher_len);
    if (!cipher || cipher_len == 0)
    {
      if (world_rank == 0)
        fprintf(stderr, "no pude leer cipher %s\n", in_bin);
      MPI_Finalize();
      return 3;
    }

    struct timespec t0 = nowMono();
    uint64_t local_iters = 0ULL;
    uint64_t found_key_local = UINT64_MAX;
    int local_found = 0;

    const uint64_t chunk = 4096ULL; // tamaño de bloque por ronda

    for (uint64_t start = (uint64_t)world_rank;
         start <= key_upper && !local_found;
         start += (uint64_t)world_size * chunk)
    {

      uint64_t end = start + (uint64_t)world_size * chunk - 1ULL;
      if (end > key_upper)
        end = key_upper;

      for (uint64_t k = start + (uint64_t)world_rank; k <= end; k += (uint64_t)world_size)
      {
        unsigned char *dec = NULL;
        size_t dec_len = 0;
        if (decryptDesEcb(k, cipher, cipher_len, &dec, &dec_len) == 0)
        {
          ++local_iters;
          if (containsPhrase(dec, dec_len, frase))
          {
            local_found = 1;
            found_key_local = k;
            free(dec);
            break;
          }
        }
        if (dec)
          free(dec);
      }

      int any_found = 0;
      MPI_Allreduce(&local_found, &any_found, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
      if (any_found)
        break;
    }

    uint64_t total_iters = 0ULL;
    MPI_Allreduce(&local_iters, &total_iters, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, MPI_COMM_WORLD);

    uint64_t found_key_global = UINT64_MAX;
    MPI_Allreduce(&found_key_local, &found_key_global, 1, MPI_UNSIGNED_LONG_LONG, MPI_MIN, MPI_COMM_WORLD);

    int my_rank_if_found = local_found ? world_rank : 1000000000;
    int finder_rank = 0, tmp_min_rank = 0;
    MPI_Allreduce(&my_rank_if_found, &tmp_min_rank, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
    finder_rank = (found_key_global == UINT64_MAX) ? 0 : tmp_min_rank;

    struct timespec t1 = nowMono();
    time_span_t dt = diffMono(t0, t1);

    if (world_rank == 0)
    {
      if (found_key_global != UINT64_MAX)
      {
        fprintf(stdout, "%llu encontrado; tiempo=%.6f s; iters=%llu; rank=%d\n",
                (unsigned long long)found_key_global, dt.secs,
                (unsigned long long)total_iters, finder_rank);
      }
      else
      {
        fprintf(stdout, "no encontrado; tiempo=%.6f s; iters=%llu\n",
                dt.secs, (unsigned long long)total_iters);
      }
      char ts[32];
      isoUtcNow(ts, sizeof ts);
      writeCsvRow(csv_path, "impl1", "decrypt",
                  (unsigned long long)key_upper,
                  p_param, dt.secs, total_iters,
                  (found_key_global != UINT64_MAX) ? 1 : 0,
                  finder_rank, ts, hostname, frase, "", in_bin);
    }

    free(cipher);
    MPI_Finalize();
    return (found_key_global != UINT64_MAX) ? 0 : 1;
  }

  if (world_rank == 0)
    printUsage(argv[0]);
  MPI_Finalize();
  return 2;
}
