# Catálogo de funciones — Implementación 3 (Impl3)

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
