#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

typedef struct
{
  double secs;
  long nsec;
} time_span_t;

static inline struct timespec nowMono()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts;
}

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

static inline void isoUtcNow(char *buf, size_t n)
{
  time_t t = time(NULL);
  struct tm g;
  gmtime_r(&t, &g);
  strftime(buf, n, "%Y-%m-%dT%H:%M:%SZ", &g);
}

#endif