/*
  impl2_par.c — Implementación 2 (PARALELA MPI) = altA
  - Reparto block-cyclic sin maestro.
  - Stop no bloqueante (broadcast por TAG_FOUND entre pares).
  - Rank 0 agrega TODAS las filas (una por rank) en: data/impl2/par.csv
  - Encabezado fijo:
    implementation,key,p,repetition,time_seconds,iterations_done,found,finder_rank,timestamp,hostname,phrase,text
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <mpi.h>

#include "../include/common.h"
#include "../include/core_utils.h"
#include "../include/core_crypto.h"

#define TAG_FOUND 77
#define TAG_ROW 99

typedef struct
{
  unsigned long long key;
  int p;
  int repetition;
  double time_seconds;
  unsigned long long iterations_done;
  int found;
  int finder_rank;
  char ts[64];
  char hostname[64];
} csv_row_t;

static void usage(const char *p)
{
  fprintf(stderr, "uso (par): mpirun -np <P> %s encrypt <key56> <out_bin> <csv_path> <hostname>\n", p);
  fprintf(stderr, "           mpirun -np <P> %s decrypt \"<frase>\" <key_upper> <p> <csv_path> <hostname> <in_bin>\n", p);
}

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (argc < 2)
  {
    if (rank == 0)
      usage(argv[0]);
    MPI_Finalize();
    return 2;
  }
  const char *mode = argv[1];

  if (strcmp(mode, "encrypt") == 0)
  {
    if (argc < 6)
    {
      if (rank == 0)
        usage(argv[0]);
      MPI_Finalize();
      return 2;
    }

    uint64_t key56 = strtoull(argv[2], NULL, 10);
    const char *out_bin = argv[3];
    const char *csv = argv[4];
    const char *host = argv[5];

    size_t plain_len = 0;
    unsigned char *plain = NULL;
    if (rank == 0)
    {
      plain = readAllStdin(&plain_len);
      if (!plain || plain_len == 0)
      {
        fprintf(stderr, "stdin vacio\n");
        MPI_Abort(MPI_COMM_WORLD, 3);
      }
    }
    MPI_Bcast(&plain_len, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
    if (rank != 0)
      plain = (unsigned char *)malloc(plain_len);
    if (plain_len > 0)
      MPI_Bcast(plain, (int)plain_len, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    struct timespec t0 = nowMono();
    unsigned char *cipher = NULL;
    size_t cipher_len = 0;
    if (rank == 0)
    {
      if (encryptDesEcb(key56, plain, plain_len, &cipher, &cipher_len) != 0)
      {
        fprintf(stderr, "encrypt fallo\n");
        free(plain);
        MPI_Abort(MPI_COMM_WORLD, 4);
      }
    }
    MPI_Bcast(&cipher_len, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
    if (rank != 0)
      cipher = (unsigned char *)malloc(cipher_len);
    if (cipher_len > 0)
      MPI_Bcast(cipher, (int)cipher_len, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
    struct timespec t1 = nowMono();
    time_span_t dt = diffMono(t0, t1);

    if (rank == 0)
    {
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
        fprintf(fp, "impl2,encrypt,%llu,%d,1,%.9f,0,0,0,%s,%s,\"\",\"%s\",%s\n",
                (unsigned long long)key56, size, dt.secs, ts, host, plain_csv, out_bin);
        free(plain_csv);
        fclose(fp);
      }
      else
      {
        fprintf(stderr, "no pude abrir csv %s: %s\n", csv, strerror(errno));
      }
    }

    free(plain);
    free(cipher);
    MPI_Finalize();
    return 0;
  }

  if (strcmp(mode, "decrypt") == 0 || strcmp(mode, "brute") == 0)
  {
    if (argc < 8)
    {
      if (rank == 0)
        usage(argv[0]);
      MPI_Finalize();
      return 2;
    }

    const char *phrase = argv[2];
    uint64_t key_upper = strtoull(argv[3], NULL, 10);
    int p_cli = atoi(argv[4]);
    const char *csv = argv[5];
    const char *host = argv[6];
    const char *in_bin = argv[7];

    size_t cipher_len = 0;
    unsigned char *cipher = NULL;
    if (rank == 0)
    {
      cipher = readFile(in_bin, &cipher_len);
      if (!cipher || cipher_len == 0)
      {
        fprintf(stderr, "no pude leer %s\n", in_bin);
        MPI_Abort(MPI_COMM_WORLD, 3);
      }
    }
    MPI_Bcast(&cipher_len, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
    if (rank != 0)
      cipher = (unsigned char *)malloc(cipher_len);
    if (cipher_len > 0)
      MPI_Bcast(cipher, (int)cipher_len, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    uint64_t iterations = 0, found_key = (uint64_t)(~0ULL);
    int found = 0, winner_rank = -1;

    struct timespec t0 = nowMono();

    for (uint64_t turn = 0;; ++turn)
    {
      int flag = 0;
      MPI_Status st;
      MPI_Iprobe(MPI_ANY_SOURCE, TAG_FOUND, MPI_COMM_WORLD, &flag, &st);
      if (flag)
      {
        uint64_t kwin;
        MPI_Recv(&kwin, 1, MPI_UNSIGNED_LONG_LONG, st.MPI_SOURCE, TAG_FOUND, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        found = 1;
        found_key = kwin;
        winner_rank = st.MPI_SOURCE;
        break;
      }

      __uint128_t base128 = (__uint128_t)rank * 1 + (__uint128_t)turn * ((__uint128_t)1 * (__uint128_t)size);
      if (base128 > key_upper)
        break;
      uint64_t base = (uint64_t)base128;
      uint64_t endb = base;
      if (endb > key_upper)
        endb = key_upper;

      for (uint64_t k = base; k <= endb; ++k)
      {
        int f2 = 0;
        MPI_Status st2;
        MPI_Iprobe(MPI_ANY_SOURCE, TAG_FOUND, MPI_COMM_WORLD, &f2, &st2);
        if (f2)
        {
          uint64_t kwin;
          MPI_Recv(&kwin, 1, MPI_UNSIGNED_LONG_LONG, st2.MPI_SOURCE, TAG_FOUND, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          found = 1;
          found_key = kwin;
          winner_rank = st2.MPI_SOURCE;
          goto FINISH;
        }
        unsigned char *dec = NULL;
        size_t dec_len = 0;
        if (decryptDesEcb(k, cipher, cipher_len, &dec, &dec_len) == 0)
        {
          iterations++;
          if (containsPhrase(dec, dec_len, phrase))
          {
            found = 1;
            found_key = k;
            winner_rank = rank;
            free(dec);
            for (int r = 0; r < size; ++r)
            {
              if (r == rank)
                continue;
              MPI_Request rq;
              MPI_Isend(&found_key, 1, MPI_UNSIGNED_LONG_LONG, r, TAG_FOUND, MPI_COMM_WORLD, &rq);
            }
            goto FINISH;
          }
        }
        free(dec);
      }
    }

  FINISH:;
    time_span_t dt = diffMono(t0, nowMono());

    if (rank == 0)
    {
      if (found)
      {
        printf("%llu encontrado; tiempo=%.6f s; iters_local=%llu\n",
               (unsigned long long)found_key, dt.secs, (unsigned long long)iterations);
      }
      else
      {
        printf("no encontrado; tiempo=%.6f s; iters_local=%llu\n",
               dt.secs, (unsigned long long)iterations);
      }
    }

    csv_row_t row;
    row.key = (found_key == (uint64_t)(~0ULL) ? (unsigned long long)key_upper : (unsigned long long)found_key);
    row.p = size;
    row.repetition = 1;
    row.time_seconds = dt.secs;
    row.iterations_done = (unsigned long long)iterations;
    row.found = found;
    row.finder_rank = (winner_rank < 0 ? -1 : winner_rank);
    isoUtcNow(row.ts, sizeof row.ts);
    snprintf(row.hostname, sizeof row.hostname, "%s", host);

    if (rank != 0)
    {
      MPI_Send(&row, sizeof(row), MPI_BYTE, 0, TAG_ROW, MPI_COMM_WORLD);
    }
    else
    {
      FILE *fp = fopen(csv, "a+");
      if (!fp)
      {
        fprintf(stderr, "no pude abrir csv %s: %s\n", csv, strerror(errno));
        MPI_Abort(MPI_COMM_WORLD, 5);
      }
      ensureHeader(fp, CSV_HEADER);

      char *phrase_csv = csvSanitize((const unsigned char *)phrase, strlen(phrase));
      /* rank 0 */
      fprintf(fp,
              "impl2,decrypt,%llu,%d,1,%.9f,%llu,%d,%d,%s,%s,%s,\"\",%s\n",
              row.key, row.p, row.time_seconds, row.iterations_done, row.found, row.finder_rank, row.ts, row.hostname, phrase_csv, in_bin);

      for (int i = 1; i < size; ++i)
      {
        csv_row_t rcv;
        MPI_Recv(&rcv, sizeof(rcv), MPI_BYTE, MPI_ANY_SOURCE, TAG_ROW, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        fprintf(fp,
                "impl2,decrypt,%llu,%d,1,%.9f,%llu,%d,%d,%s,%s,%s,\"\",%s\n",
                rcv.key, rcv.p, rcv.time_seconds, rcv.iterations_done, rcv.found, rcv.finder_rank, rcv.ts, rcv.hostname, phrase_csv, in_bin);
      }
      free(phrase_csv);
      fclose(fp);
    }

    free(cipher);
    MPI_Finalize();
    return found ? 0 : 1;
  }

  if (rank == 0)
    usage(argv[0]);
  MPI_Finalize();
  return 2;
}
