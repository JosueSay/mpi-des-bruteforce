# Formato de Data

Este documento establece **estándares para la organización de la data del proyecto**.

## 1. Estructura de Carpetas

La data de cada implementación se debe organizar de la siguiente manera:

```bash
data/
├── impl1/               # Implementación 1: naive
│   ├── sec.csv          # Datos secuenciales
│   └── par.csv          # Datos paralelos
├── impl2/               # Implementación 2: alternativa A
│   ├── sec.csv
│   └── par.csv
└── impl3/               # Implementación 3: alternativa B
    ├── sec.csv
    └── par.csv
```

- Cada carpeta contiene **los resultados completos de esa implementación**.
- `sec.csv`: ejecución **secuencial**, 1 fila por llave.
- `par.csv`: ejecución **paralela**, varias filas por llave y por número de procesadores `p`.

## 2. Formato de CSV

Todos los CSV deben seguir estrictamente este formato de columnas:

```csv
implementation,mode,key,p,repetition,time_seconds,iterations_done,found,finder_rank,timestamp,hostname,phrase,text,out_bin
```

Ejemplo (encrypt y decrypt):

```csv
impl1,encrypt,123456,0,1,0.000007453,0,0,0,2025-10-29T18:34:33Z,pc_josue,"","Esta es una prueba de proyecto 2","/path/to/cipher.bin"
impl1,decrypt,123456,1,1,0.044451413,123457,1,0,2025-10-29T18:34:37Z,pc_josue,"es una prueba de","","/path/to/cipher.bin"
```

### Descripción de cada columna

| Columna           | Descripción                                                                                                                        |
| ----------------- | ---------------------------------------------------------------------------------------------------------------------------------- |
| `implementation`  | Nombre de la implementación ("naive", "altA", "altB").                                                                             |
| `mode`            | Modo de la ejecución (`encrypt` o `decrypt`).                                                                                      |
| `key`             | Llave probada (entera). Para llaves grandes, guardar como número sin comillas.                                                     |
| `p`               | Número de procesos usados (entero). Secuencial siempre `1`. Para operaciones de *encrypt* que no usan procesos, `p` puede ser `0`. |
| `repetition`      | Índice de repetición para la misma combinación (1,2,3…).                                                                           |
| `time_seconds`    | Tiempo total de ejecución en segundos (float).                                                                                     |
| `iterations_done` | Número de claves intentadas en esa ejecución.                                                                                      |
| `found`           | Indicador si se encontró la llave (1 = sí, 0 = no).                                                                                |
| `finder_rank`     | Rank del proceso que encontró la llave (0 si raíz o -1 si no se encontró).                                                         |
| `timestamp`       | Instante de inicio o fin en formato ISO 8601 UTC (`YYYY-MM-DDTHH:MM:SSZ`).                                                         |
| `hostname`        | Nombre de la máquina donde se ejecutó.                                                                                             |
| `phrase`          | Frase o palabra clave usada para validar el descifrado.                                                                            |
| `text`            | Texto original (plano o cifrado) procesado durante la ejecución.                                                                   |
| `out_bin`         | Ruta al archivo binario de salida.                                                                                                 |

> **Reglas sobre `phrase` y `text`:**
>
> - Si `mode == "encrypt"`: **`text`** contiene el texto a cifrar; **`phrase`** queda vacío.
> - Si `mode == "decrypt"`: **`phrase`** contiene la frase usada para validar el descifrado; **`text`** queda vacío.
> - Todas las columnas **son obligatorias**; si un campo no aplica, poner cadena vacía `""` (no NULL).

## 3. Variables y métricas para el reporte

### Tiempo Secuencial ($T_s$)

- **Definición:** Tiempo total de ejecución en un solo procesador.
- **Uso:** Base para cálculo de speedup.
- **Obtención:** Medición de la ejecución secuencial (`sec.csv`).

### Tiempo Paralelo ($T_p$)

- **Definición:** Tiempo total usando múltiples procesadores.
- **Uso:** Comparar con $T_s$ para medir aceleración.
- **Obtención:** Medición paralela con diferentes `p` (`par.csv`).

### Número de Procesadores ($p$)

- **Definición:** Cantidad de unidades de procesamiento usadas.
- **Uso:** Para calcular speedup y eficiencia.
- **Nota:** Para operaciones no-paralelas (por ejemplo `encrypt` que solo exporta un bin), usar `p = 0` y documentarlo en el análisis.
