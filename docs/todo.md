# TODO detallado — **repositorio:** `mpi-des-bruteforce`

## Pasos iniciales

1. Crear repo: `mpi-des-bruteforce`.
2. Estructura de carpetas (crear ya):

    ```bash
    mpi-des-bruteforce/
    ├─ src/
    ├─ docs/
    ├─ evidencia/
    │  ├─ capturas/
    │  └─ timings/
    ├─ reports/
    ├─ scripts/
    ├─ Makefile
    ├─ README.md
    └─ LICENSE
    ```

3. Añadir `README.md` mínimo con propósito y comandos básicos (compilar, ejecutar).
4. Definir llaves de prueba y textos (fichero `tests/keys.txt`, `tests/input.txt`) — usar: la frase a buscar: `"es una prueba de"`.
5. Instalar dependencias en README (OpenMPI, librería DES alternativa si aplica).
6. Crear plantilla de bitácora individual: `docs/bitacora_<usuario>.md` (cada integrante usará su propia).
7. Crear plantilla de formato para mediciones: `evidencia/timings/template.csv` (cols: experimento, impl, np, llave, tiempo_s, timestamp, notas).

## Roles (3 personas — tareas separadas, sin solaparse)

> Nota: cada rol produce su **código**, su **bitácora de funciones** y sus **evidencias**. No mezclar implementaciones entre roles.

### Rol A — **Implementación 1 (Naive)**

**Objetivo:** implementar la versión base `naive` secuencial y paralela.

- Archivos:

  - `src/impl1_seq.c` — recorrido secuencial completo.
  - `src/impl1_par.c` — versión MPI naive (segmentos equitativos por rank).
  - Usa funciones comunes en `include/common.h` / `src/common.c` (encrypt/decrypt, tryKey, util timers).
- Requerimientos:

  - Validación de argumentos y manejo de errores.
  - Imprimir: llave encontrada, nombre archivo, palabra clave.
  - Guardar logs en `evidencia/capturas/impl1_*`.
- Bitácora:

  - `docs/bitacora_RolA.md`: listar funciones implementadas con entradas/salidas y ejemplos de uso (mínimo las funciones públicas que creó).
- Pruebas:

  - Ejecutar con `-np 1` y `-np 4`. Guardar `evidencia/timings/impl1_*.csv`.
- Entregables de Rol A: `src/impl1_seq.c`, `src/impl1_par.c`, `docs/bitacora_RolA.md`, capturas y timings.

### Rol B — **Implementación 2 (Alternativa 1)**

**Objetivo:** diseñar e implementar primera alternativa al naive (ej.: reparto intercalado, o maestro/trabajador dinámico).

- Archivos:

  - `src/impl2_seq.c` — versión secuencial del mismo enfoque (para comparar).
  - `src/impl2_par.c` — versión MPI de este approach.
- Requerimientos:

  - Documentar algoritmo (pseudocódigo en comentarios y en bitácora).
  - Medir tiempos y comparar con impl1 (usar los mismos casos de prueba).
- Bitácora:

  - `docs/bitacora_RolB.md` con catálogo de funciones implementadas (entradas/salidas/purpose).
- Pruebas:

  - Ejecutar con `-np 4` y otros `-np` relevantes. Guardar capturas y `evidencia/timings/impl2_*.csv`.
- Entregables de Rol B: `src/impl2_seq.c`, `src/impl2_par.c`, `docs/bitacora_RolB.md`, capturas y timings.

### Rol C — **Implementación 3 (Alternativa 2) + Infraestructura**

**Objetivo:** segunda alternativa (ej.: búsqueda aleatoria con seeds por proceso, o work-stealing simple) **y** mantener common library, Makefile, scripts de ejecución y recolección de datos.

- Archivos y tareas:

  - `src/impl3_seq.c`, `src/impl3_par.c`.
  - `include/common.h` y `src/common.c`: contener encrypt/decrypt, tryKey, util de timing, lectura archivo input, búsqueda de substring; **todos** los impl deben usar estas funciones (evitar duplicados).
  - `Makefile` completo con targets:

    - `make all`, `make impl1_seq`, `make impl1_par`, `make impl2_*`, `make impl3_*`, `make clean`.
  - `scripts/run_experiments.sh`: automatiza ejecuciones para N procesos, llaves dadas, y guarda CSVs y logs en `evidencia/timings/`.
  - `scripts/collect_results.py` (opcional) para combinar CSVs.
- Bitácora:

  - `docs/bitacora_RolC.md` con catálogo de funciones del `common` y de impl3.
- Pruebas infra:

  - Verificar que `mpirun -np 4 bin/implX_par ...` funcione y que outputs vayan a `evidencia/`.
- Entregables de Rol C: `src/common.*`, `Makefile`, `scripts/`, `docs/bitacora_RolC.md`, capturas y timings.

## Reglas para evitar solapamiento

- `common.*` lo edita **solo Rol C**. Resto usan esa API para no duplicar código.
- Cada rol documenta **solo** sus funciones en su `bitacora_*`. Si modifican `common`, Rol C actualiza todas las bitácoras con nota.
- Nombres de funciones públicas deben seguir prefijo `impl1_`, `impl2_`, `impl3_` o `common_` para evitar colisiones.
- Pull requests: cada rol trabaja en su rama y abre PR a `main`. Revisiones cruzadas (1 miembro revisa PR de otro).

## Tareas grupales (unir y entregar)

Estas tareas se hacen **conjuntamente** cuando el código y evidencias individuales estén listas.

1. **Informe (`reports/informe.pdf`)** — contener:

   - Carátula, índice, introducción.
   - Antecedentes conceptuales DES y diagrama de flujo (Parte A1-2).
   - Metodología: describir cada implementación (incluye pseudocódigo y diagramas por implementación).
   - Resultados: tablas de timings y speedups (usar CSVs de `evidencia/timings/`).
   - Discusión: comportamiento según llaves (fácil/medio/difícil), consistencia y recomendaciones.
   - Conclusiones y apéndices (catálogos de funciones — aquí enlazar a las `bitacora_*.md` y/o resumir).
2. **Diagrama de flujo**: una por algoritmo (naive, alt1, alt2) — guardar en `reports/diagrams/`.
3. **Unir bitácoras**: incorporar extractos de `docs/bitacora_*.md` en Anexo 1 del informe (cada quien aporta su parte).
4. **Anexo 2 — Bitácora de pruebas**: compilar todos los CSV en una tabla comparativa; incluir capturas.
5. **Make target final**: `make deliver` que genere un ZIP con `reports/`, `src/`, `README.md`, `evidencia/`.
6. **Revisión final**: ejecutar `make all`, correr ejemplos en README, asegurar que todas las rutas de evidencia funcionen.

## Entregables finales (lista concreta)

- `reports/informe.pdf` (completo).
- `src/` con: `impl1_seq.c`, `impl1_par.c`, `impl2_*`, `impl3_*`, `common.*`.
- `Makefile`, `scripts/run_experiments.sh`.
- `README.md` con instrucciones de compilación/ejecución.
- `docs/bitacora_RolA.md`, `docs/bitacora_RolB.md`, `docs/bitacora_RolC.md`.
- `evidencia/timings/*.csv` y `evidencia/capturas/*.png`.
