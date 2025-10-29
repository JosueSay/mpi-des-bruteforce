# Catálogo de funciones utilitarias que auxilian a las implementaciones

## Módulo `core_crypto.c` / `core_crypto.h`

Funciones criptográficas basadas en **DES en modo ECB** (OpenSSL).

### Función `encryptDesEcb`

Cifra un bloque de datos usando DES-ECB con padding tipo PKCS#7. ey56** — `uint64_t` — Clave de 56 bits efectivos.

- **in** — `const unsigned char *` — Datos de entrada.
- **len** — `size_t` — Longitud del texto de entrada.
- **out** — `unsigned char **` — Puntero de salida (memoria asignada internamente).
- **out_len** — `size_t *` — Longitud resultante del cifrado.

#### Salidas

- **Código de retorno** — `int` — `0` si éxito, `-1` en caso de error.
- **`out`** contiene el texto cifrado.

### Función `decryptDesEcb`

Descifra datos DES-ECB y elimina el padding.  

#### Entradas

- ** Clave usada para descifrar.
- **in** — `const unsigned char *` — Datos cifrados.
- **len** — `size_t` — Longitud del bloque (múltiplo de 8).
- **out** — `unsigned char **` — Salida asignada dinámicamente.
- **out_len** — `size_t *` — Longitud del texto plano.

#### Salidas

- **Código de retorno** — `int` — `0` si éxito, `-1` si error o padding inválido.

### Función `containsPhrase`

Busca una subcadena dentro de un buffer binario (ya descifrado).  

#### Entradas

- **buf** — `const unsignnalizar.
- **len** — `size_t` — Longitud del texto.
- **phrase** — `const char *` — Frase a buscar.

#### Salidas

- **Retorno** — `int` — `1` si la frase se encuentra, `0` si no o error.

## Módulo `core_utils.c` / `core_utils.h`

Funciones de utilidad general: tiempo, I/O binario y manejo CSV.

### Función `nowMono`

Devuelve la hora actual usando reloj monotónico (no afectado por ajustes de sistema).  

#### Entradas

- **Ninguna**

#### Salidas

- **Valor** — `str actual.

### Función `diffMono`

Calcula la diferencia entre dos marcas de tiempo (`a → b`).  

#### Entradas

- **a**, **b** — `struct timespec` — Tiempos de referencia.

#### S `time_span_t` — Intervalo en segundos y nanosegundos

### Función `isoUtcNow`

Escribe en un buffer la fecha/hora UTC en formato ISO 8601.  

#### Entradas

- **buf** — `char *` — Buffer destino.
- **n** — `size_t` — Tamaño del buffer.

#### Salodifica `buf` con una cadena como `"2025-10-29T23:59:59Z"`

### Función `ensureHeader`

Garantiza que un archivo CSV contenga el encabezado inicial.  

#### Entradas

- **fp** — `FILE *` — Archivo abierto.
- **header_csv** — `const char *` — Encabezado a verificar/escribir.

na**

### Función `readAllStdin`

Lee completamente la entrada estándar (`stdin`) a memoria dinámica.  

#### Entradas

- **out_len** — `size_t *` — Donde se guarda la longitud leída.

#### Salidas

- **Retorno** — `unsigned char *` — Buffer con los di error.

### Función `readFile`

Lee un archivo binario completo en memoria.  

#### Entradas

- **path** — `const char *` — Ruta del archivo.
- **out_len** — `size_t *` — Longitud leída.

#### Salidas

- **Retorno** — `unsigned char *` — Buffer csi falla.

### Función `writeFile`

Escribe un buffer binario en disco.  

#### Entradas

- **path** — `const char *` — Ruta destino.
- **buf** — `const unsigned char *` — Datos a escribir.
- **len** — `size_t` — Longitud de datos.

#### Salidas

- **Retorno** — `-1` si error.

### Función `csvSanitize`

Prepara texto para incluirlo en un campo CSV: duplica com1illas, elimina saltos de línea.  

#### Entradas

- **in** — `const unsigned char *` — Cadena de entrada.
- **len** — `size_t` — Longitud del texto.

#### Salidas

- **Retorno** — `char *` — Cadena nueva limpia y segura para CSV.
