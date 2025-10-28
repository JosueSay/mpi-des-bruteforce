#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "../include/common.h"
#include "../include/impl3.h"

static const char *CSV_HEADER =
"implementation,key,p,repetition,time_seconds,iterations_done,found,finder_rank,timestamp,hostname,phrase,text";

// lectura total de stdin
static unsigned char *readAllStdin(size_t *out_len) {
    size_t cap = 4096, len = 0;
    unsigned char *buf = malloc(cap);
    if (!buf) return NULL;
    for (;;) {
        size_t n = fread(buf + len, 1, cap - len, stdin);
        len += n;
        if (n == 0) break;
        if (len == cap) {
            cap *= 2;
            unsigned char *t = realloc(buf, cap);
            if (!t) { free(buf); return NULL; }
            buf = t;
        }
    }
    *out_len = len;
    return buf;
}

static void printUsage(const char *p) {
    fprintf(stderr, "uso: echo \"<texto>\" | %s <frase> <key> <p> <csv_path> <hostname>\n", p);
}

int main(int argc, char **argv) {
    if (argc < 6) {
        printUsage(argv[0]);
        return 2;
    }

    const char *phrase   = argv[1];
    uint64_t key_true    = strtoull(argv[2], NULL, 10);
    int p                = atoi(argv[3]);
    const char *csv_path = argv[4];
    const char *hostname = argv[5];

    size_t plain_len = 0;
    unsigned char *plain = readAllStdin(&plain_len);
    if (!plain || plain_len == 0) {
        fprintf(stderr, "stdin vacio\n");
        return 3;
    }

    unsigned char *cipher = NULL;
    size_t cipher_len = 0;
    encryptDesEcb(key_true, plain, plain_len, &cipher, &cipher_len);

    struct timespec t0 = nowMono();
    uint64_t iterations = 0;
    int found = 0;
    uint64_t found_key = UINT64_MAX;

    for (uint64_t k = 0; k <= key_true; ++k) {
        unsigned char *dec=NULL;
        size_t dec_len=0;
        decryptDesEcb(k, cipher, cipher_len, &dec, &dec_len);
        iterations++;
        if (containsPhrase(dec, dec_len, phrase)) {
            found = 1;
            found_key = k;
            free(dec);
            break;
        }
        free(dec);
    }

    struct timespec t1 = nowMono();
    time_span_t dt = diffMono(t0, t1);

    printf(found ? "found=%llu\n" : "no encontrado\n", (unsigned long long)found_key);

    FILE *fp = fopen(csv_path, "a+");
    if (fp) {
        ensureHeader(fp, CSV_HEADER);
        char ts[64]; isoUtcNow(ts, sizeof ts);
        char *plain_c = malloc(plain_len+1);
        memcpy(plain_c, plain, plain_len);
        plain_c[plain_len]='\0';

        fprintf(fp,
        "impl3,%llu,%d,%d,%.9f,%llu,%d,%d,%s,%s,\"%s\",\"%s\"\n",
        (unsigned long long)found_key,
        p, 1, dt.secs,
        (unsigned long long)iterations,
        found, 0, ts, hostname, phrase, plain_c);

        free(plain_c);
        fclose(fp);
    }

    free(plain);
    free(cipher);
    return 0;
}
