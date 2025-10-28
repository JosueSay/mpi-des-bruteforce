#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// tiempo medido
typedef struct
{
  double secs;
  long nsec;
} time_span_t;

// retorna tiempo monotónico actual
static inline struct timespec nowMono(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts;
}

// diferencia entre dos tiempos monotónicos (a -> b)
static inline time_span_t diffMono(struct timespec a, struct timespec b)
{
  time_span_t d;
  long sec = b.tv_sec - a.tv_sec;
  long nsec = b.tv_nsec - a.tv_nsec;
  if (nsec < 0)
  {
    --sec;
    nsec += 1000000000L;
  }
  d.secs = (double)sec + (double)nsec / 1e9;
  d.nsec = nsec;
  return d;
}

// escribe timestamp ISO UTC en buf
static inline void isoUtcNow(char *buf, size_t n)
{
  time_t t = time(NULL);
  struct tm g;
  gmtime_r(&t, &g);
  strftime(buf, n, "%Y-%m-%dT%H:%M:%SZ", &g);
}

// cabecera CSV usada por los logs
static const char *CSV_HEADER =
    "implementation,mode,key,p,repetition,time_seconds,iterations_done,found,"
    "finder_rank,timestamp,hostname,phrase,text,in_path,out_path";

// asegura que el archivo CSV tenga header si está vacío
static inline void ensureHeader(FILE *fp, const char *expected_header)
{
  long pos = ftell(fp);
  fseek(fp, 0, SEEK_END);
  long end = ftell(fp);
  if (end == 0)
  {
    fprintf(fp, "%s\n", expected_header);
  }
  fseek(fp, pos, SEEK_SET);
}

// modos de ejecución
typedef enum
{
  runModeEncrypt = 0,
  runModeDecrypt = 1,
  runModeCrackSeq = 2,
  runModeCrackPar = 3
} run_mode_t;

// lee archivo completo en memoria
static inline int readEntireFile(const char *path, unsigned char **buf, size_t *len)
{
  *buf = NULL;
  *len = 0;
  FILE *f = fopen(path, "rb");
  if (!f)
    return -1;
  if (fseek(f, 0, SEEK_END) != 0)
  {
    fclose(f);
    return -1;
  }
  long sz = ftell(f);
  if (sz < 0)
  {
    fclose(f);
    return -1;
  }
  rewind(f);
  unsigned char *tmp = (unsigned char *)malloc((size_t)sz + 1);
  if (!tmp)
  {
    fclose(f);
    return -1;
  }
  size_t rd = fread(tmp, 1, (size_t)sz, f);
  fclose(f);
  if (rd != (size_t)sz)
  {
    free(tmp);
    return -1;
  }
  tmp[sz] = 0;
  *buf = tmp;
  *len = (size_t)sz;
  return 0;
}

// escribe buffer completo en archivo
static inline int writeEntireFile(const char *path, const unsigned char *buf, size_t len)
{
  FILE *f = fopen(path, "wb");
  if (!f)
    return -1;
  size_t wr = fwrite(buf, 1, len, f);
  fclose(f);
  return (wr == len) ? 0 : -1;
}

#endif
