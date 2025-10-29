#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "../include/common.h"
#include "../include/core_utils.h"
#include "../include/core_crypto.h"

static void printUsage(const char *p)
{
    fprintf(stderr, "uso (seq): %s encrypt <key56> <out_bin> <csv_path> <hostname>\n", p);
    fprintf(stderr, "           %s decrypt \"<frase>\" <key_upper> <p> <csv_path> <hostname> <in_bin>\n", p);
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printUsage(argv[0]);
        return 2;
    }
    const char *mode = argv[1];

    // ---------- encrypt ----------
    if (strcmp(mode, "encrypt") == 0)
    {
        if (argc < 6)
        {
            printUsage(argv[0]);
            return 2;
        }

        uint64_t key56 = strtoull(argv[2], NULL, 10);
        const char *out_bin = argv[3];
        const char *csv_path = argv[4];
        const char *hostname = argv[5];

        size_t plain_len = 0;
        unsigned char *plain = readAllStdin(&plain_len);
        if (!plain || plain_len == 0)
        {
            fprintf(stderr, "stdin vacio\n");
            free(plain);
            return 3;
        }

        struct timespec t0 = nowMono();
        unsigned char *cipher = NULL;
        size_t cipher_len = 0;
        if (encryptDesEcb(key56, plain, plain_len, &cipher, &cipher_len) != 0)
        {
            fprintf(stderr, "encrypt fallo\n");
            free(plain);
            return 4;
        }
        struct timespec t1 = nowMono();
        time_span_t dt = diffMono(t0, t1);

        if (writeFile(out_bin, cipher, cipher_len) != 0)
        {
            fprintf(stderr, "no pude escribir %s\n", out_bin);
        }

        FILE *fp = fopen(csv_path, "a+");
        if (fp)
        {
            ensureHeader(fp, CSV_HEADER);
            char ts[64];
            isoUtcNow(ts, sizeof ts);
            char *plain_csv = csvSanitize(plain, plain_len);

            fprintf(fp, "impl3,encrypt,%llu,0,1,%.9f,0,0,0,%s,%s,\"\",\"%s\",%s\n",
                    (unsigned long long)key56, dt.secs, ts, hostname, plain_csv, out_bin);
            free(plain_csv);
            fclose(fp);
        }
        else
        {
            fprintf(stderr, "no pude abrir csv %s\n", csv_path);
        }

        free(plain);
        free(cipher);
        return 0;
    }

    if (strcmp(mode, "decrypt") == 0 || strcmp(mode, "brute") == 0)
    {
        if (argc < 8)
        {
            printUsage(argv[0]);
            return 2;
        }

        const char *phrase = argv[2];
        uint64_t key_upper = strtoull(argv[3], NULL, 10);
        int p = atoi(argv[4]);
        const char *csv_path = argv[5];
        const char *hostname = argv[6];
        const char *in_bin = argv[7];

        size_t cipher_len = 0;
        unsigned char *cipher = readFile(in_bin, &cipher_len);
        if (!cipher || cipher_len == 0)
        {
            fprintf(stderr, "no pude leer cipher %s\n", in_bin);
            free(cipher);
            return 3;
        }

        struct timespec t0 = nowMono();
        uint64_t iterations = 0;
        int found = 0;
        uint64_t found_key = UINT64_MAX;

        for (uint64_t k = 0; k <= key_upper; ++k)
        {
            unsigned char *dec = NULL;
            size_t dec_len = 0;
            if (decryptDesEcb(k, cipher, cipher_len, &dec, &dec_len) == 0)
            {
                iterations++;
                if (containsPhrase(dec, dec_len, phrase))
                {
                    found = 1;
                    found_key = k;
                    free(dec);
                    break;
                }
            }
            free(dec);
        }

        struct timespec t1 = nowMono();
        time_span_t dt = diffMono(t0, t1);

        if (found)
            printf("found=%llu\n", (unsigned long long)found_key);
        else
            printf("no encontrado\n");

        FILE *fp = fopen(csv_path, "a+");
        if (fp)
        {
            ensureHeader(fp, CSV_HEADER);
            char ts[64];
            isoUtcNow(ts, sizeof ts);

            fprintf(fp,
                    "impl3,decrypt,%llu,%d,1,%.9f,%llu,%d,%d,%s,%s,\"%s\",\"\",%s\n",
                    (unsigned long long)key_upper,
                    p, dt.secs,
                    (unsigned long long)iterations,
                    found, 0,
                    ts, hostname, phrase, in_bin);
            fclose(fp);
        }
        else
        {
            fprintf(stderr, "no pude abrir csv %s\n", csv_path);
        }

        free(cipher);
        return found ? 0 : 1;
    }

    printUsage(argv[0]);
    return 2;
}
