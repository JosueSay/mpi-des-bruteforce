# Anexo 1 — Catálogo de funciones y librerías

## Función `printUsage` — **paralela** (`impl1_par.c`)

Imprime en `stderr` las instrucciones de uso para la versión MPI.

### Entradas

- **prog**
  - **Tipo:** `const char *`
  - **Descripción:** Nombre del programa (`argv[0]`).

### Salidas

- **Ninguna**
  - **Tipo:** `void`
  - **Descripción:** Muestra modos `encrypt` y `decrypt` con su sintaxis.

## Función `writeCsvRow` — **paralela** (`impl1_par.c`)

Escribe una fila en el CSV con el encabezado estándar, asegurando que exista.

### Entradas

- **csv_path** — `const char *` — Ruta del CSV.
- **implementation** — `const char *` — Identificador (p. ej., `"impl1"`).
- **mode** — `const char *` — `"encrypt"` o `"decrypt"`.
- **key** — `unsigned long long` — Clave (o límite superior).
- **p** — `int` — Parámetro informativo (p. ej., procesos).
- **secs** — `double` — Tiempo total en segundos.
- **iterations_done** — `unsigned long long` — Iteraciones ejecutadas.
- **found** — `int` — Indicador 0/1 de hallazgo.
- **finder_rank** — `int` — Rank del proceso que encontró la clave.
- **timestamp** — `const char *` — Marca de tiempo ISO.
- **hostname** — `const char *` — Nombre del host.
- **phrase** — `const char *` — Frase buscada (vacía en `encrypt`).
- **text** — `const char *` — Texto plano (en `encrypt`).
- **out_bin** — `const char *` — Ruta del binario cifrado (en `encrypt`).

### Salidas

- **Ninguna**
  - **Tipo:** `void`
  - **Descripción:** Agrega una línea formateada al CSV.

## Función `main` — **paralela MPI** (`impl1_par.c`)

Punto de entrada de la versión **paralela**. Maneja `encrypt` y `decrypt`/`brute` distribuidos.

### Entradas

- **argc** — `int` — Número de argumentos.
- **argv** — `char **` — Vector de argumentos.

#### Modo `encrypt`

1. `encrypt`
2. `key` — Clave numérica.
3. `out_bin` — Archivo binario de salida.
4. `csv` — Ruta del CSV.
5. `hostname`

#### Modo `decrypt`/`brute`

1. `decrypt` o `brute`
2. `frase` — Texto a buscar.
3. `key_upper` — Límite superior de clave.
4. `p` — Parámetro informativo (p. ej., `np`).
5. `csv` — Ruta del CSV.
6. `hostname`
7. `in_bin` — Archivo cifrado de entrada.

### Salidas

- **Código de retorno** — `int`  
  `0` éxito (o clave encontrada), `1` no encontrada, `2–6` errores de uso/E/S.

### Descripción de funcionamiento

- Inicializa MPI y obtiene `world_size`/`world_rank`.  
- **Encrypt (`rank 0`)**: lee `stdin`, cifra, escribe `out_bin`, registra CSV, imprime tiempo.
- **Decrypt**: reparto por bloques con *stride* por `world_size`; cada proceso descifra claves intercaladas dentro de ventanas (`chunk = 4096`). Reducciones con `MPI_Allreduce` para iteraciones totales, clave mínima encontrada y *rank* del ganador; `rank 0` imprime y escribe CSV.

## Función `printUsage` — **secuencial** (`impl1_seq.c`)

Imprime en `stderr` las instrucciones de uso de la versión secuencial.

### Entradas

- **prog** — `const char *` — Nombre del programa.

### Salidas

- **Ninguna** — `void` — Muestra sintaxis de `encrypt` y `decrypt`.

## Función `main` — **secuencial** (`impl1_seq.c`)

Punto de entrada de la versión **secuencial**. Soporta `encrypt` y `decrypt`/`brute`.

### Entradas

- **argc** — `int`  
- **argv** — `char **`

#### Modo `encrypt`

1. `encrypt`
2. `key` — Clave numérica.
3. `out_bin` — Archivo binario de salida.
4. `csv` — Ruta del CSV.
5. `hostname`

#### Modo `decrypt`/`brute`

1. `decrypt` o `brute`
2. `frase` — Texto a buscar.
3. `key_upper` — Límite superior de clave.
4. `p` — Parámetro informativo (solo registro).
5. `csv` — Ruta del CSV.
6. `hostname`
7. `in_bin` — Archivo cifrado de entrada.

### Salidas

- **Código de retorno** — `int`  
  `0` éxito; `1` no encontrada; `2–6` errores de uso/archivo.

### Descripción de funcionamiento

- **Encrypt**: valida que `out_bin` no exista, lee `stdin`, cifra, escribe binario, registra CSV y tiempos.
- **Decrypt**: lee `in_bin`, recorre claves `0..key_upper`, cuenta iteraciones, detecta frase, imprime resultado y registra CSV.

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

## Constantes (etiquetas MPI)

- **TAG_REQ = 1**
  - **Tipo:** `#define`  
  - **Descripción:** Señal que envían los *workers* al maestro para solicitar un nuevo segmento de claves.

- **TAG_CHUNK = 2**
  - **Tipo:** `#define`  
  - **Descripción:** Respuesta del maestro con el rango `[inicio, fin]` a procesar; `UINT64_MAX, UINT64_MAX` indica detenerse.

- **TAG_FOUND = 3**
  - **Tipo:** `#define`  
  - **Descripción:** Mensaje del *worker* al maestro notificando que encontró la clave y cuántas iteraciones realizó.

## Función `printUsage` — **paralela** (`impl3_par.c`)

Imprime en `stderr` las instrucciones de uso para la versión MPI.

### Entradas

- **p**
  - **Tipo:** `const char *`
  - **Descripción:** Nombre del programa (`argv[0]`).

### Salidas

- **Ninguna**
  - **Tipo:** `void`
  - **Descripción:** Muestra los modos `encrypt` y `decrypt`.

## Función `printUsage` — **secuencial** (`impl3_seq.c`)

Imprime en `stderr` las instrucciones de uso para la versión secuencial.

### Entradas

- **p**
  - **Tipo:** `const char *`
  - **Descripción:** Nombre del programa (`argv[0]`).

### Salidas

- **Ninguna**
  - **Tipo:** `void`
  - **Descripción:** Muestra los modos `encrypt` y `decrypt`.

## Función `main` — **secuencial** (`impl3_seq.c`)

Punto de entrada de la versión **secuencial**. Soporta `encrypt` y `decrypt`/`brute`.

### Entradas

- **argc**
  - **Tipo:** `int`
  - **Descripción:** Número de argumentos.
- **argv**
  - **Tipo:** `char **`
  - **Descripción:** Vector de argumentos.

#### Argumentos esperados (modo `encrypt`)

1. `encrypt`  
2. `key56` — clave numérica.  
3. `out_bin` — archivo binario de salida.  
4. `csv_path` — ruta al CSV.  
5. `hostname`.

#### Argumentos esperados (modo `decrypt`/`brute`)

1. `decrypt` o `brute`  
2. `frase` — texto a buscar.  
3. `key_upper` — límite superior de búsqueda.  
4. `p` — parámetro informativo.  
5. `csv_path`.  
6. `hostname`.  
7. `in_bin` — archivo cifrado de entrada.

### Salidas

- **Código de retorno**
  - **Tipo:** `int`
  - **Descripción:** `0` éxito; `1` no se encontró clave; `2–4` errores de uso/operación.

### Descripción de funcionamiento

- Lee `stdin` (en `encrypt`) o el binario (`in_bin`) en `decrypt`.  
- Mide tiempo, ejecuta cifrado/descifrado, verifica la frase, imprime resultado y registra una fila en el CSV. :contentReference[oaicite:6]{index=6}

## Función `main` — **paralela MPI** (`impl3_par.c`)

Punto de entrada de la versión **paralela** con modelo **maestro–worker** dinámico.

### Entradas

- **argc**
  - **Tipo:** `int`
  - **Descripción:** Número de argumentos.
- **argv**
  - **Tipo:** `char **`
  - **Descripción:** Vector de argumentos.

#### Modo `encrypt`

1. `encrypt`  
2. `key56` — clave numérica.  
3. `out_bin` — archivo binario de salida.  
4. `csv_path` — ruta al CSV.  
5. `hostname`.

#### Modo `decrypt`/`brute`

1. `decrypt` o `brute`  
2. `frase` — texto a buscar.  
3. `key_upper` — límite superior de búsqueda.  
4. `p` — parámetro informativo.  
5. `csv_path`.  
6. `hostname`.  
7. `in_bin` — archivo cifrado de entrada.

### Salidas

- **Código de retorno**
  - **Tipo:** `int`
  - **Descripción:** `0` si se encontró/operó con éxito; `1` si no se encontró clave; `2` o códigos de aborto para errores.

### Descripción de funcionamiento

- Inicializa MPI y difunde datos de entrada.  
- **`encrypt`**: `rank 0` cifra, mide, escribe binario y fila CSV.
- **`decrypt`**:
  - **Maestro (`rank 0`)**: asigna rangos de claves por demanda (*work pulling*). Atiende `TAG_REQ` enviando `[inicio, fin]` vía `TAG_CHUNK`; ante `TAG_FOUND` difunde paradas con `[UINT64_MAX, UINT64_MAX]`.
  - **Workers (`rank > 0`)**: piden trabajo (`TAG_REQ`), descifran el rango recibido (`TAG_CHUNK`), notifican hallazgo con `TAG_FOUND` (clave + iteraciones) y se detienen.
  - Consolida métricas con `MPI_Allreduce` (iteraciones totales, clave mínima encontrada y *rank* del ganador) y registra en CSV.
