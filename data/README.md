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

> Esta organización permite automatizar análisis y cálculos de speedup y eficiencia sin mezclar implementaciones ni resultados.

## 2. Formato de CSV

Todos los CSV deben seguir estrictamente este formato:

```csv
implementation,key,p,repetition,time_seconds,iterations_done,found,finder_rank,timestamp,hostname
naive,123456,1,1,12.345,123456,1,0,2025-10-21T14:32:00Z,mihost
```

### Descripción de cada columna

| Columna           | Descripción                                                                                      |
| ----------------- | ------------------------------------------------------------------------------------------------ |
| `implementation`  | Nombre de la implementación ("naive", "altA", "altB").                                           |
| `key`             | Llave probada (entera). Para llaves grandes, guardar como número sin comillas.                   |
| `p`               | Número de procesos usados (entero). Secuencial siempre `1`.                                      |
| `repetition`      | Índice de repetición para la misma combinación (1,2,3…).                                         |
| `time_seconds`    | Tiempo total de ejecución en segundos (float).                                                   |
| `iterations_done` | Número de claves intentadas en esa ejecución.                                                    |
| `found`           | Indicador si se encontró la llave (1 = sí, 0 = no).                                              |
| `finder_rank`     | Rank del proceso que encontró la llave (0 si raíz o -1 si no se encontró). Mantener consistente. |
| `timestamp`       | Instante de inicio o fin en formato ISO 8601 UTC (`YYYY-MM-DDTHH:MM:SSZ`).                       |
| `hostname`        | Nombre de la máquina donde se ejecutó.                                                           |

> Todos los campos son obligatorios; no se dejan libres ni opcionales.

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
