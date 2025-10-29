#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

// csv header compartido por todas las implementaciones
static const char *CSV_HEADER =
    "implementation,mode,key,p,repetition,time_seconds,iterations_done,found,finder_rank,timestamp,hostname,phrase,text,out_bin";

typedef struct
{
  double secs;
  long nsec;
} time_span_t;

// devuelve timestamp monotónico
static inline struct timespec nowMono()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts;
}

// diferencia entre dos tiempos monotónicos en segundos + nsec
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

// escribe header si el archivo está vacío
static inline void ensureHeader(FILE *fp, const char *header_csv)
{
  long pos = ftell(fp);
  fseek(fp, 0, SEEK_END);
  long end = ftell(fp);
  if (end == 0)
  {
    fprintf(fp, "%s\n", header_csv);
  }
  fseek(fp, pos, SEEK_SET);
}

// escribe timestamp UTC ISO8601 en buf
static inline void isoUtcNow(char *buf, size_t n)
{
  time_t t = time(NULL);
  struct tm g;
  gmtime_r(&t, &g);
  strftime(buf, n, "%Y-%m-%dT%H:%M:%SZ", &g);
}

#endif // COMMON_H
