# Catálogo de funciones — Implementación 2 (Impl2)

## Función `printUsage`

Imprime en `stderr` las instrucciones de uso para la versión secuencial (`impl2_seq`).

### Entradas

- **p**
  - **Tipo:** `const char *`
  - **Descripción:** Nombre del programa (normalmente `argv[0]`).

### Salidas

- **Ninguna**
  - **Tipo:** `void`
  - **Descripción:** Muestra texto de ayuda en la consola de error estándar.

## Función `usage`

Imprime las instrucciones de uso para la versión paralela MPI (`impl2_par`).

### Entradas

- **p**
  - **Tipo:** `const char *`
  - **Descripción:** Nombre del programa (normalmente `argv[0]`).

### Salidas

- **Ninguna**
  - **Tipo:** `void`
  - **Descripción:** Muestra texto explicativo con los modos `encrypt` y `decrypt`.

## Función `main` — `impl2_seq.c`

Función principal para la ejecución **secuencial** del programa.  
Permite cifrar (`encrypt`) o descifrar por fuerza bruta (`decrypt` o `brute`) usando DES-ECB.

### Entradas

- **argc**
  - **Tipo:** `int`
  - **Descripción:** Número de argumentos pasados por CLI.

- **argv**
  - **Tipo:** `char **`
  - **Descripción:** Vector con los argumentos del programa.  

#### Argumentos esperados (modo `encrypt`)

1. `encrypt`
2. `key56` — clave numérica de 56 bits.
3. `out_bin` — ruta del archivo binario de salida.
4. `csv_path` — ruta del archivo CSV.
5. `hostname` — nombre del host.

#### Argumentos esperados (modo `decrypt`)

1. `decrypt` o `brute`
2. `frase` — texto buscado dentro del descifrado.
3. `key_upper` — límite superior de búsqueda de clave.
4. `p` — parámetro informativo (número de procesos, en secuencial suele ser 1).
5. `csv_path` — ruta del archivo CSV.
6. `hostname`
7. `in_bin` — archivo cifrado de entrada.

### Salidas

- **Código de salida del proceso**
  - **Tipo:** `int`
  - **Descripción:**  
    - `0` si la operación fue exitosa.  
    - `1` si no se encontró la clave.  
    - `2–5` para errores de uso, lectura o escritura.

## Función `main` — `impl2_par.c`

Función principal de la implementación **paralela con MPI**.  
Permite cifrar (`encrypt`) y descifrar por fuerza bruta distribuida (`decrypt`/`brute`) en múltiples procesos.

### Entradas

- **argc**
  - **Tipo:** `int`
  - **Descripción:** Número de argumentos proporcionados al ejecutar el binario.

- **argv**
  - **Tipo:** `char **`
  - **Descripción:** Lista de argumentos, interpretados según el modo (`encrypt` o `decrypt`).

#### Modo `encrypt`

1. `encrypt`
2. `key56` — clave numérica.
3. `out_bin` — archivo binario cifrado.
4. `csv_path` — ruta al archivo CSV.
5. `hostname`.

#### Modo `decrypt`

1. `decrypt` o `brute`
2. `frase` — texto a buscar.
3. `key_upper` — límite de clave.
4. `p` — parámetro informativo.
5. `csv_path`.
6. `hostname`.
7. `in_bin` — archivo cifrado de entrada.

### Salidas

- **Código de retorno**
  - **Tipo:** `int`
  - **Descripción:**
    - `0`: éxito o clave encontrada.
    - `1`: clave no encontrada.
    - `2–5`: errores de argumentos, lectura o escritura.

### Descripción del funcionamiento

1. Inicializa MPI (`MPI_Init`, `MPI_Comm_rank`, `MPI_Comm_size`).
2. En **modo `encrypt`**:
   - `rank 0` lee el texto plano, cifra con DES, y difunde el resultado.
   - Se escribe el archivo binario y se añade una fila al CSV.
3. En **modo `decrypt`**:
   - Todos los procesos reciben el `cipher` por difusión.
   - Cada proceso explora subconjuntos de claves usando reparto *block-cyclic*.
   - Si un proceso encuentra la frase, envía `TAG_FOUND` a los demás.
   - Se reducen resultados con `MPI_Reduce` para consolidar métricas.
   - `rank 0` escribe la fila consolidada al CSV.

## Estructura `csv_row_t`

Estructura auxiliar usada internamente en `impl2_par.c` para representar una fila del CSV.

### Campos

- **key**
  - **Tipo:** `unsigned long long`
  - **Descripción:** Clave probada o encontrada.

- **p**
  - **Tipo:** `int`
  - **Descripción:** Número de procesos.

- **repetition**
  - **Tipo:** `int`
  - **Descripción:** Número de repetición o corrida.

- **time_seconds**
  - **Tipo:** `double`
  - **Descripción:** Tiempo total de ejecución en segundos.

- **iterations_done**
  - **Tipo:** `unsigned long long`
  - **Descripción:** Cantidad de iteraciones ejecutadas.

- **found**
  - **Tipo:** `int`
  - **Descripción:** Indicador (0/1) de si se encontró la frase.

- **finder_rank**
  - **Tipo:** `int`
  - **Descripción:** Rank del proceso que encontró la clave.

- **ts**
  - **Tipo:** `char[64]`
  - **Descripción:** Timestamp ISO.

- **hostname**
  - **Tipo:** `char[64]`
  - **Descripción:** Nombre del host donde se ejecutó.
