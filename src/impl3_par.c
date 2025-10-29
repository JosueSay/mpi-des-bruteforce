#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <unistd.h>
#include <stdint.h>

#include "../include/common.h"
#include "../include/core_utils.h"
#include "../include/core_crypto.h"

#define TAG_REQ 1
#define TAG_CHUNK 2
#define TAG_FOUND 3

static void printUsage(const char *p)
{
    fprintf(stderr, "uso (par): mpirun -np P %s encrypt <key56> <out_bin> <csv_path> <hostname>\n", p);
    fprintf(stderr, "       mpirun -np P %s decrypt \"<frase>\" <key_upper> <p> <csv_path> <hostname> <in_bin>\n", p);
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int rank = 0, size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 2)
    {
        if (rank == 0)
            printUsage(argv[0]);
        MPI_Finalize();
        return 2;
    }

    const char *mode = argv[1];

    if (strcmp(mode, "encrypt") == 0)
    {
        if (argc < 6)
        {
            if (rank == 0)
                printUsage(argv[0]);
            MPI_Finalize();
            return 2;
        }

        uint64_t key56 = strtoull(argv[2], NULL, 10);
        const char *out_bin = argv[3];
        const char *csv_path = argv[4];
        const char *hostname = argv[5];

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
        {

            plain = (unsigned char *)malloc(plain_len ? plain_len : 1);
            if (!plain)
                MPI_Abort(MPI_COMM_WORLD, 12);
        }
        MPI_Bcast(plain, (int)plain_len, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

        unsigned char *cipher = NULL;
        size_t cipher_len = 0;
        if (rank == 0)
        {
            if (encryptDesEcb(key56, plain, plain_len, &cipher, &cipher_len) != 0)
            {
                fprintf(stderr, "encrypt fallo\n");
                MPI_Abort(MPI_COMM_WORLD, 4);
            }

            if (writeFile(out_bin, cipher, cipher_len) != 0)
            {
                fprintf(stderr, "no pude escribir %s\n", out_bin);
            }
        }

        if (rank == 0)
        {
            FILE *fp = fopen(csv_path, "a+");
            if (!fp)
            {
                fprintf(stderr, "no pude abrir csv %s\n", csv_path);
            }
            else
            {
                ensureHeader(fp, CSV_HEADER);
                char ts[64];
                isoUtcNow(ts, sizeof ts);
                char *plain_csv = csvSanitize(plain, plain_len);

                struct timespec t0 = nowMono();
                struct timespec t1 = nowMono();
                time_span_t dt = diffMono(t0, t1);
                fprintf(fp, "impl3,encrypt,%llu,0,1,%.9f,0,0,0,%s,%s,\"\",\"%s\",%s\n",
                        (unsigned long long)key56,
                        dt.secs,
                        ts, hostname, plain_csv, out_bin);
                free(plain_csv);
                fclose(fp);
            }
            free(cipher);
        }

        free(plain);
        MPI_Finalize();
        return 0;
    }
    else if (strcmp(mode, "decrypt") == 0 || strcmp(mode, "brute") == 0)
    {
        if (argc < 8)
        {
            if (rank == 0)
                printUsage(argv[0]);
            MPI_Finalize();
            return 2;
        }

        const char *phrase = argv[2];
        uint64_t key_upper = strtoull(argv[3], NULL, 10);
        int p_param = atoi(argv[4]);
        const char *csv_path = argv[5];
        const char *hostname = argv[6];
        const char *in_bin = argv[7];

        size_t cipher_len = 0;
        unsigned char *cipher = NULL;
        if (rank == 0)
        {
            cipher = readFile(in_bin, &cipher_len);
            if (!cipher || cipher_len == 0)
            {
                fprintf(stderr, "no pude leer cipher %s\n", in_bin);
                MPI_Abort(MPI_COMM_WORLD, 3);
            }
        }

        MPI_Bcast(&cipher_len, 1, MPI_UNSIGNED_LONG, 0, MPI_COMM_WORLD);
        if (rank != 0)
        {
            cipher = (unsigned char *)malloc(cipher_len ? cipher_len : 1);
            if (!cipher)
                MPI_Abort(MPI_COMM_WORLD, 12);
        }
        MPI_Bcast(cipher, (int)cipher_len, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

        uint64_t next = 0;
        uint64_t found_key_local = UINT64_MAX;
        uint64_t found_key_global = UINT64_MAX;
        uint64_t iters_local = 0;
        uint64_t iters_global = 0;
        int local_found = 0;
        int global_any_found = 0;

        struct timespec t0 = nowMono();

        if (rank == 0)
        {
            int active = size - 1;
            uint64_t chunk = (key_upper / (uint64_t)size);
            if (chunk < 1)
                chunk = 1;

            MPI_Status st;
            while (active > 0 && !global_any_found)
            {
                MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &st);
                int src = st.MPI_SOURCE;
                int tag = st.MPI_TAG;

                if (tag == TAG_REQ)
                {
                    MPI_Recv(NULL, 0, MPI_BYTE, src, TAG_REQ, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    if (global_any_found || next > key_upper)
                    {
                        uint64_t stopmsg[2] = {UINT64_MAX, UINT64_MAX};
                        MPI_Send(stopmsg, 2, MPI_UNSIGNED_LONG_LONG, src, TAG_CHUNK, MPI_COMM_WORLD);
                        active--;
                    }
                    else
                    {
                        uint64_t seg[2];
                        seg[0] = next;
                        uint64_t e = next + chunk - 1;
                        if (e > key_upper)
                            e = key_upper;
                        seg[1] = e;
                        next = e + 1;
                        MPI_Send(seg, 2, MPI_UNSIGNED_LONG_LONG, src, TAG_CHUNK, MPI_COMM_WORLD);
                    }
                }
                else if (tag == TAG_FOUND)
                {
                    uint64_t payload[2];
                    MPI_Recv(payload, 2, MPI_UNSIGNED_LONG_LONG, src, TAG_FOUND, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    global_any_found = 1;
                    found_key_global = payload[0];
                    iters_global = payload[1];

                    uint64_t stopmsg[2] = {UINT64_MAX, UINT64_MAX};
                    for (int r = 1; r < size; r++)
                        MPI_Send(stopmsg, 2, MPI_UNSIGNED_LONG_LONG, r, TAG_CHUNK, MPI_COMM_WORLD);
                }
            }
        }
        else
        {
            int stop = 0;
            while (!stop)
            {
                MPI_Send(NULL, 0, MPI_BYTE, 0, TAG_REQ, MPI_COMM_WORLD);

                uint64_t seg[2];
                MPI_Recv(seg, 2, MPI_UNSIGNED_LONG_LONG, 0, TAG_CHUNK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                if (seg[0] == UINT64_MAX)
                {
                    stop = 1;
                    break;
                }

                for (uint64_t k = seg[0]; k <= seg[1]; ++k)
                {
                    unsigned char *dec = NULL;
                    size_t dec_len = 0;
                    if (decryptDesEcb(k, cipher, cipher_len, &dec, &dec_len) == 0)
                    {
                        iters_local++;
                        if (containsPhrase(dec, dec_len, phrase))
                        {
                            local_found = 1;
                            found_key_local = k;
                            uint64_t payload[2] = {k, iters_local};
                            MPI_Send(payload, 2, MPI_UNSIGNED_LONG_LONG, 0, TAG_FOUND, MPI_COMM_WORLD);
                            free(dec);
                            stop = 1;
                            break;
                        }
                    }
                    free(dec);
                }
            }
        }

        MPI_Allreduce(&iters_local, &iters_global, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, MPI_COMM_WORLD);
        MPI_Allreduce(&found_key_local, &found_key_global, 1, MPI_UNSIGNED_LONG_LONG, MPI_MIN, MPI_COMM_WORLD);

        struct timespec t1 = nowMono();
        time_span_t dt = diffMono(t0, t1);
        char ts[64];
        isoUtcNow(ts, sizeof ts);

        char *plain_text_csv = NULL;

        char csvpath[256];
        snprintf(csvpath, sizeof(csvpath), "%s/impl3_par_rank%d.csv", csv_path, rank);
        FILE *fp = fopen(csvpath, "a+");
        if (!fp)
        {
            if (rank == 0)
                fprintf(stderr, "no pude abrir csv %s\n", csvpath);
        }
        else
        {
            ensureHeader(fp, CSV_HEADER);
            uint64_t key_out = (found_key_global != UINT64_MAX) ? found_key_global : key_upper;
            int found_flag = (found_key_global != UINT64_MAX) ? 1 : 0;
            int finder_rank = found_flag ? rank : 0;
            char *text_print = plain_text_csv ? plain_text_csv : "";
            fprintf(fp, "impl3,decrypt,%llu,%d,1,%.9f,%llu,%d,%d,%s,%s,\"%s\",\"%s\",\n",
                    (unsigned long long)key_out,
                    p_param,
                    dt.secs,
                    (unsigned long long)iters_global,
                    found_flag,
                    (found_flag ? finder_rank : 0),
                    ts, hostname, phrase, text_print);
            fclose(fp);
        }

        free(plain_text_csv);
        free(cipher);
        MPI_Finalize();
        return (found_key_global != UINT64_MAX) ? 0 : 1;
    }
    else
    {
        if (rank == 0)
            printUsage(argv[0]);
        MPI_Finalize();
        return 2;
    }
}
