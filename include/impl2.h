#ifndef IMPL2_H
#define IMPL2_H
/*
  impl2.h
  =========
  Propósito:
    - Declarar la interfaz mínima que usa Impl2 (igual contrato que Impl1/Impl3)
      para cifrar/descifrar y buscar la frase dentro del texto plano.

  Notas:
    - Si compilas con -DUSE_OPENSSL y enlazas -lcrypto, se usa DES real (ECB).
    - Si no, se utiliza un XOR simple como “toy cipher” para pruebas y mediciones
      (sirve para medir Speedup/Eficiencia sin trabas de librerías).
*/
#include <stddef.h>
#include <stdint.h>

int encryptDesEcb(uint64_t key56,
                  const unsigned char *in, size_t len,
                  unsigned char **out, size_t *out_len);

int decryptDesEcb(uint64_t key56,
                  const unsigned char *in, size_t len,
                  unsigned char **out, size_t *out_len);

int containsPhrase(const unsigned char *buf, size_t len,
                   const char *phrase);

#endif /* IMPL2_H */
