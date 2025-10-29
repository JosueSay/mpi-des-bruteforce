# Catálogo de funciones y librerías — Implementación 2 (impl2 / altA)

> Esta implementación ofrece modos `encrypt` y `decrypt/brute` en versiones **secuencial** y  **paralela (MPI)** , con salida en CSV estandarizada. Las firmas públicas de Impl2 están en `impl2.h`.
>
> La CLI y la escritura CSV para **secuencial** están en `impl2_seq.c`.
>
> La lógica **paralela (MPI)** y su CSV están en `impl2_par.c`.

---

## encryptDesEcb

Implementa el **cifrado** (DES/ECB con padding 8-bytes si se compila con `-DUSE_OPENSSL -lcrypto`; de lo contrario, XOR simple para pruebas). Declarada en `impl2.h`; usada en `impl2_seq.c` (modo `encrypt`) y `impl2_par.c` (modo `encrypt`).

### Entradas

* **key56** — `uint64_t`

  Clave de 56 bits (se pasa en un entero de 64).
* **in** — `const unsigned char *`

  Buffer de texto plano.
* **len** — `size_t`

  Longitud del texto plano.

### Salidas

* **out** — `unsigned char **`

  Puntero a buffer con texto cifrado (se asigna dentro).
* **out_len** — `size_t *`

  Longitud del cifrado.
* **return** — `int`

  `0` si ok; distinto de 0 si falla.

---

## decryptDesEcb

Implementa el **descifrado** (DES/ECB con unpadding si hay OpenSSL; XOR de prueba si no). Declarada en `impl2.h`; usada en `impl2_seq.c` (modo `decrypt/brute`) y `impl2_par.c` (modo `decrypt/brute`).

### Entradas

* **key56** — `uint64_t`

  Clave candidata (fuerza bruta).
* **in** — `const unsigned char *`

  Buffer cifrado.
* **len** — `size_t`

  Longitud del cifrado.

### Salidas

* **out** — `unsigned char **`

  Texto plano recuperado (asignado dentro).
* **out_len** — `size_t *`

  Longitud del texto plano.
* **return** — `int`

  `0` si ok.

---

## containsPhrase

Valida si una **frase** aparece en el texto plano descifrado. Declarada en `impl2.h`; usada en los bucles de fuerza bruta secuencial y paralelo.

### Entradas

* **buf** — `const unsigned char *`

  Texto descifrado.
* **len** — `size_t`

  Longitud del texto.
* **phrase** — `const char *`

  Frase a buscar (no vacía).

### Salidas

* **return** — `int`

  `1` si la contiene; `0` en caso contrario.

---

## printUsage / usage (CLI)

Imprime uso de la herramienta:

* **Secuencial** : `impl2_seq encrypt <key56> <out_bin> <csv_path> <hostname>` y `impl2_seq decrypt "<frase>" <key_upper> <p> <csv_path> <hostname> <in_bin>`.
* **Paralelo (MPI)** : `mpirun -np <P> impl2_par encrypt <key56> <out_bin> <csv_path> <hostname>` y `mpirun -np <P> impl2_par decrypt "<frase>" <key_upper> <p> <csv_path> <hostname> <in_bin>`.

### Entradas

* **p** — `const char *`

  Nombre del ejecutable (argv[0]).

### Salidas

* **(stdout/stderr)** — Mensaje de ayuda; sin valor de retorno significativo.

---

## main (impl2_seq)

Punto de entrada secuencial. Dos modos:

* **encrypt** : lee  **stdin** , cifra y escribe binario; agrega fila al CSV.
* **decrypt/brute** : lee cifrado, explora claves `0..key_upper`, registra métricas y CSV.

### Entradas (vía argv)

* `mode` (`"encrypt"` | `"decrypt"`/`"brute"`)
* **encrypt** : `<key56> <out_bin> <csv_path> <hostname>`
* **decrypt** : `"<frase>" <key_upper> <p> <csv_path> <hostname> <in_bin>`

### Salidas

* **stdout** — Mensajes “found=no/encontrado…”.
* **Ficheros** — `out_bin` (encrypt), fila en `csv_path`.
* **Código de salida** — `0` ok; `>0` error o no encontrado.

---

## main (impl2_par, MPI)

Punto de entrada paralelo. Dos modos:

* **encrypt** : rank 0 lee  **stdin** , difunde, cifra y escribe binario + fila CSV.
* **decrypt/brute** : rank 0 lee cifrado y difunde; todos exploran claves con **block-cyclic** (chunk 1 en esta versión), notifican **hallazgo temprano** con `TAG_FOUND`; rank 0 agrega fila CSV con **reducciones** (tiempo máx, iters totales, ganador).

### Entradas (vía argv)

* `mode` (`"encrypt"` | `"decrypt"`/`"brute"`)
* **encrypt** : `<key56> <out_bin> <csv_path> <hostname>`
* **decrypt** : `"<frase>" <key_upper> <p> <csv_path> <hostname> <in_bin>`

### Salidas

* **stdout** — Resumen del hallazgo y tiempos.
* **Ficheros** — `out_bin` (encrypt), fila en `csv_path`.
* **Código de salida** — `0` ok/encontrado; `1` no encontrado; `2` uso inválido.



## Librerías y headers

* **MPI** — `#include <mpi.h>` (solo paralelo). Comunicación: `MPI_Init`, `MPI_Comm_rank`, `MPI_Comm_size`, `MPI_Bcast`, `MPI_Iprobe`, `MPI_Recv`, `MPI_Isend`, `MPI_Reduce`, `MPI_Finalize`.
* **OpenSSL (opcional)** — DES/ECB real cuando se compila con `-DUSE_OPENSSL -lcrypto`. Si no, Impl2 puede usar fallback XOR (definido en capa de crypto). Declarado en `impl2.h`.
* **C estándar** — `stdio.h`, `stdlib.h`, `string.h`, `stdint.h`, `errno.h` (manejo de errores/archivos/strings).

## Librerías y headers

* **MPI** — `#include <mpi.h>` (solo paralelo). Comunicación: `MPI_Init`, `MPI_Comm_rank`, `MPI_Comm_size`, `MPI_Bcast`, `MPI_Iprobe`, `MPI_Recv`, `MPI_Isend`, `MPI_Reduce`, `MPI_Finalize`.
* **OpenSSL (opcional)** — DES/ECB real cuando se compila con `-DUSE_OPENSSL -lcrypto`. Si no, Impl2 puede usar fallback XOR (definido en capa de crypto). Declarado en `impl2.h`.
* **C estándar** — `stdio.h`, `stdlib.h`, `string.h`, `stdint.h`, `errno.h` (manejo de errores/archivos/strings).


## Librerías y headers

* **MPI** — `#include <mpi.h>` (solo paralelo). Comunicación: `MPI_Init`, `MPI_Comm_rank`, `MPI_Comm_size`, `MPI_Bcast`, `MPI_Iprobe`, `MPI_Recv`, `MPI_Isend`, `MPI_Reduce`, `MPI_Finalize`.
* **OpenSSL (opcional)** — DES/ECB real cuando se compila con `-DUSE_OPENSSL -lcrypto`. Si no, Impl2 puede usar fallback XOR (definido en capa de crypto). Declarado en `impl2.h`.
* **C estándar** — `stdio.h`, `stdlib.h`, `string.h`, `stdint.h`, `errno.h` (manejo de errores/archivos/strings).


## Librerías y headers

* **MPI** — `#include <mpi.h>` (solo paralelo). Comunicación: `MPI_Init`, `MPI_Comm_rank`, `MPI_Comm_size`, `MPI_Bcast`, `MPI_Iprobe`, `MPI_Recv`, `MPI_Isend`, `MPI_Reduce`, `MPI_Finalize`.
* **OpenSSL (opcional)** — DES/ECB real cuando se compila con `-DUSE_OPENSSL -lcrypto`. Si no, Impl2 puede usar fallback XOR (definido en capa de crypto). Declarado en `impl2.h`.
* **C estándar** — `stdio.h`, `stdlib.h`, `string.h`, `stdint.h`, `errno.h` (manejo de errores/archivos/strings).
