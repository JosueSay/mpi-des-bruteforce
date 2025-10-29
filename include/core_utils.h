#ifndef CORE_UTILS_H
#define CORE_UTILS_H

#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include "common.h"

// tiempo y csv
struct timespec nowMono(void);                              // timestamp monotónico
time_span_t diffMono(struct timespec a, struct timespec b); // diferencia a->b
void isoUtcNow(char *buf, size_t n);                        // utc iso8601
void ensureHeader(FILE *fp, const char *header_csv);        // escribe header si vacío

// io util
unsigned char *readAllStdin(size_t *out_len);                          // lee stdin completo (malloc)
unsigned char *readFile(const char *path, size_t *out_len);            // lee archivo bin
int writeFile(const char *path, const unsigned char *buf, size_t len); // escribe bin
char *csvSanitize(const unsigned char *in, size_t len);                // duplica " y limpia \r/\n

#endif
