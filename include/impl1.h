
#ifndef IMPL1_H
#define IMPL1_H

#include <stddef.h>
#include <stdint.h>
#include "common.h"

// primitivas DES ECB (implementación existente)
int encryptDesEcb(uint64_t key56, const unsigned char *in, size_t len,
                  unsigned char **out, size_t *out_len);
int decryptDesEcb(uint64_t key56, const unsigned char *in, size_t len,
                  unsigned char **out, size_t *out_len);

// busca si buf contiene phrase
int containsPhrase(const unsigned char *buf, size_t len, const char *phrase);

// encripta archivo de entrada y escribe cipher
int impl1EncryptFile(uint64_t key56, const char *in_path, const char *out_path);

// desencripta archivo cipher a texto (out_path puede ser NULL para stdout)
int impl1DecryptFile(uint64_t key56, const char *in_path, const char *out_path);

// crack secuencial sobre cipher_path; devuelve 0 si encontró la llave
int impl1CrackFileSeq(const char *cipher_path,
                      const char *phrase,
                      uint64_t start_key,
                      uint64_t end_key,
                      uint64_t *found_key,
                      uint64_t *iterations_done,
                      double *time_seconds);

// crack paralelo (MPI): world_size y world_rank deben proveerse
int impl1CrackFilePar(const char *cipher_path,
                      const char *phrase,
                      uint64_t start_key,
                      uint64_t end_key,
                      int world_size,
                      int world_rank,
                      uint64_t *found_key,
                      uint64_t *iterations_done_local,
                      int *found_rank,
                      double *time_seconds);

// escribe una fila en CSV con el esquema extendido
int impl1LogCsv(FILE *csv,
                const char *implementation,
                run_mode_t mode,
                uint64_t key,
                int p,
                int repetition,
                double time_seconds,
                uint64_t iterations_done,
                int found,
                int finder_rank,
                const char *timestamp,
                const char *hostname,
                const char *phrase,
                const char *text,
                const char *in_path,
                const char *out_path);

#endif
