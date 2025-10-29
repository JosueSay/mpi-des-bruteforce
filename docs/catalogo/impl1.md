# Catálogo de funciones — Implementación 1 (Impl1)

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
