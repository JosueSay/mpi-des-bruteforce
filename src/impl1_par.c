#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/stat.h>
#include <mpi.h>

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

// duplica " y normaliza \r/\n para CSV
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

// muestra uso
static void printUsage(const char *prog)
{
  fprintf(stderr, "uso:\n");
  fprintf(stderr, "  encrypt: echo \"<texto>\" | mpirun -np N %s encrypt <key> <out_bin> <csv> <hostname>\n", prog);
  fprintf(stderr, "  decrypt: mpirun -np N %s decrypt \"<frase>\" <key_upper> <p> <csv> <hostname> <in_bin>\n", prog);
  fprintf(stderr, "ejemplo encrypt:\n  echo \"Esta es una prueba\" | mpirun -np 1 %s encrypt 123456 cipher.bin data/impl1/par.csv myhost\n", prog);
  fprintf(stderr, "ejemplo decrypt:\n  mpirun -np 4 %s decrypt \"es una prueba de\" 2000000 4 data/impl1/par.csv myhost cipher.bin\n", prog);
}

// escribe una fila CSV estándar
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
  ensureHeader(fp, CSV_HEADER); // header compartido
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

  // ---------- encrypt (se ejecuta solo en rank 0 para evitar salidas duplicadas) ----------
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
      char *text_csv = csvSanitize(plain, plain_len); // guarda texto de entrada sanitizado
      writeCsvRow(csv_path, "impl1", "encrypt",
                  (unsigned long long)key_true,
                  world_size, dt.secs, 0ULL, 0, 0, ts, hostname,
                  "", text_csv, out_bin); // phrase vacío, text=plaintext
      free(text_csv);
      free(cipher);
      free(plain);
      // imprime pequeño resumen
      fprintf(stdout, "ok; tiempo=%.6f s; out=%s\n", dt.secs, out_bin);
    }

    MPI_Finalize();
    return 0;
  }

  // ---------- decrypt (alias: brute para compat) ----------
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
    if ((!cipher || cipher_len == 0))
    {
      if (world_rank == 0)
        fprintf(stderr, "no pude leer cipher %s\n", in_bin);
      MPI_Finalize();
      return 3;
    }

    // reparto simple por salto: k = rank, rank+p, ...
    struct timespec t0 = nowMono();
    uint64_t local_iters = 0ULL;
    uint64_t found_key_local = UINT64_MAX;
    int local_found = 0;

    // comprobación cooperativa: reducimos flags cada cierto bloque para abortar pronto
    const uint64_t chunk = 4096ULL;

    for (uint64_t start = (uint64_t)world_rank; start <= key_upper && !local_found; start += (uint64_t)world_size * chunk)
    {
      uint64_t end = start + (uint64_t)world_size * chunk - 1ULL;
      if (end > key_upper)
        end = key_upper;

      // iteración intercalada por rank dentro del bloque
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

      // sincronización: si alguien encontró, todos paran
      int any_found = 0;
      MPI_Allreduce(&local_found, &any_found, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
      if (any_found)
      {
        local_found = local_found ? 1 : 0;
        break;
      }
    }

    // agregados globales
    uint64_t total_iters = 0ULL;
    MPI_Allreduce(&local_iters, &total_iters, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, MPI_COMM_WORLD);

    // key y rank del que encontró
    uint64_t found_key_global = UINT64_MAX;
    // tomamos el mínimo key encontrado entre ranks (o UINT64_MAX si nadie)
    MPI_Allreduce(&found_key_local, &found_key_global, 1, MPI_UNSIGNED_LONG_LONG, MPI_MIN, MPI_COMM_WORLD);

    int my_rank_if_found = local_found ? world_rank : 1e9;
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
      // phrase con comillas, text vacío; mode siempre "decrypt"
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
