#include "../include/core_utils.h"
#include <stdlib.h>
#include <string.h>

// usa clock monotónico para métricas
struct timespec nowMono(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts;
}

// calcula diferencia a->b en segundos y nanosegundos
time_span_t diffMono(struct timespec a, struct timespec b)
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

// escribe hora actual en utc formateada
void isoUtcNow(char *buf, size_t n)
{
  time_t t = time(NULL);
  struct tm g;
  gmtime_r(&t, &g);
  strftime(buf, n, "%Y-%m-%dT%H:%M:%SZ", &g);
}

// asegura que el archivo csv tenga encabezado
void ensureHeader(FILE *fp, const char *header_csv)
{
  long pos = ftell(fp);
  fseek(fp, 0, SEEK_END);
  long end = ftell(fp);
  if (end == 0)
  {
    fprintf(fp, "%s\n", header_csv);
    fflush(fp);
  }
  fseek(fp, pos, SEEK_SET);
}

// lee stdin completo a memoria
unsigned char *readAllStdin(size_t *out_len)
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

// lee archivo binario completo
unsigned char *readFile(const char *path, size_t *out_len)
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
  unsigned char *buf = (unsigned char *)malloc((size_t)s);
  if (!buf)
  {
    fclose(f);
    return NULL;
  }
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

// escribe buffer binario en disco
int writeFile(const char *path, const unsigned char *buf, size_t len)
{
  FILE *f = fopen(path, "wb");
  if (!f)
    return -1;
  size_t w = fwrite(buf, 1, len, f);
  fclose(f);
  return (w == len) ? 0 : -1;
}

// duplica comillas y elimina saltos para campo csv estable
char *csvSanitize(const unsigned char *in, size_t len)
{
  size_t extra = 0;
  for (size_t i = 0; i < len; ++i)
    if (in[i] == '"')
      ++extra;
  char *out = (char *)malloc(len + extra + 1);
  if (!out)
    return NULL;
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
