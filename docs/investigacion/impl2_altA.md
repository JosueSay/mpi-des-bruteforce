# Implementación 2 (altA) — Investigación técnica

## 1. Modelo criptográfico

### 1.1 DES en modo ECB (con OpenSSL)

- **Bloque**: 64 bits; **clave efectiva**: 56 bits.  
- **Modo**: ECB (*Electronic Code Book*): cada bloque se cifra de forma independiente.  
- **Padding**: convencionalmente múltiplos de 8 bytes (p. ej., PKCS#7).  
- **API (vía `core_crypto.h`)**:  

  ```c
  int encryptDesEcb(uint64_t key56,
                    const unsigned char *in, size_t len,
                    unsigned char **out, size_t *out_len);

  int decryptDesEcb(uint64_t key56,
                    const unsigned char *in, size_t len,
                    unsigned char **out, size_t *out_len);

  int containsPhrase(const unsigned char *buf, size_t len,
                     const char *phrase);
  ```

Retornan `0` si la operación fue correcta y asignan el búfer de salida.

### 1.2 Detección de texto válido

- Se busca una **frase** dentro del texto plano descifrado:

  - `containsPhrase(dec, dec_len, phrase)` → `1` si la contiene, `0` en caso contrario.

## 2. Interfaz pública de Impl2 (`impl2.h`)

- **Contrato común**: el mismo que emplean Impl1/Impl3.
- **Memoria**: `out` es asignado internamente y el **llamador libera**.

## 3. Diseño secuencial (`impl2_seq.c`)

### 3.1 Flujo “encrypt”

1. Lee **stdin** → `plain`.
2. Cifra con `key56` → `cipher`.
3. Escribe binario (`out_bin`) y **agrega una fila** al CSV (`data/impl2/sec.csv`).

### 3.2 Flujo “decrypt/brute”

1. Carga **cifrado** (`in_bin`).
2. Inicia cronómetro (`nowMono()` / `diffMono`).
3. Recorre $k = 0..key_upper$:

   - Descifra → `dec`.
   - `containsPhrase(dec, dec_len, phrase)` → *found*.
   - Si encuentra: *break*.
4. Imprime en stdout (`found=…` / `no encontrado`).
5. Escribe fila en CSV:

   - `time_seconds` = tiempo total.
   - `iterations_done` = iteraciones ejecutadas.
   - `found` = 0/1.
   - `p` = valor pasado por CLI (para la tabla).

> La escritura en CSV usa utilidades de `common.h`/`core_utils.h`: `ensureHeader`, `isoUtcNow`, `csvSanitize`.

## 4. Diseño paralelo MPI (`impl2_par.c`)

### 4.1 Roles y datos compartidos

- **Todos los procesos** participan de la búsqueda (no hay maestro dedicado).
- **Difusión inicial**:

  - `encrypt`: `rank 0` lee `stdin` y difunde `plain` → cifra → escribe `out_bin` + CSV.
  - `decrypt`: `rank 0` lee `in_bin` y **difunde** `cipher` (tamaño y contenido) a todos.

### 4.2 Esquema de reparto (block-cyclic simple)

Se itera por turnos $t = 0, 1, 2, …$ y cada proceso $r$ toma claves intercaladas:

$$
\text{base}(t, r) = r \cdot c + t \cdot (c \cdot P)
$$

con **chunk** $c = 1$ y $P$ = número de procesos.

Rango local:

$$
[\text{base}, \min(\text{base} + c - 1, key_upper)]
$$

> Este patrón reparte uniformemente las claves y **reduce zonas calientes**; con $c>1$ (extensión futura) se amortizan latencias.

### 4.3 Parada temprana no bloqueante

- Cualquier proceso que detecta la **frase**:

  1. Marca `found=1`, `found_key=k`, `winner_rank=r`.
  2. Envía **no bloqueante** `TAG_FOUND` con la clave a todos.
- Todos los procesos sondean con `MPI_Iprobe(TAG_FOUND)` antes/durante el lazo:

  - Si llega `TAG_FOUND`, recuperan la clave ganadora y salen.

### 4.4 Consolidación de métricas

- `MPI_Reduce`:

  - **Tiempo**: `MAX` de los locales → `max_time`.
  - **Iteraciones**: `SUM` → `total_iters`.
  - **Found**: `MAX` (0/1) → `found_any`.
  - **Winner_rank**: `MAX` (o -1) → `global_winner`.
- `rank 0` imprime resumen y escribe la fila única al CSV `data/impl2/par.csv`.

## 5. Complejidad y modelo de desempeño

### 5.1 Complejidad básica

- **Secuencial**:

$$
T_s \approx \sum_{k=0}^{key_upper} (t_{dec}(k) + t_{chk}(k))
$$

(hasta encontrar o agotar).

- **Paralelo** (idealmente):

$$
T_p \approx \frac{T_s}{P}
$$

pero en la práctica se ve afectado por:

- **Desequilibrio** (hallazgo temprano/tardío).
- **Overheads** de comunicación (`Bcast`, `Iprobe/Isend`, `Reduce`).
- **I/O CSV** centralizado en `rank 0`.

### 5.2 *Speedup* y eficiencia

$$
S = \frac{T_s}{T_p}, \qquad E = \frac{S}{P}
$$

- **Amdahl** (fracción secuencial $f$):

$$
S_{\max} = \frac{1}{f + \frac{(1-f)}{P}}
$$

- **Gustafson** (escala con $P$):

$$
S_G \approx P - f \cdot (P - 1)
$$

> La parada temprana introduce **variabilidad**: cuando la clave está cerca del inicio, el *speedup* observado puede ser menor; si está cerca del final, la paralelización luce mejor.

## 6. Validación de correctitud

1. **Cifrado/Descifrado**: con `-DUSE_OPENSSL` debe recuperarse el `cipher` con la misma clave.
2. **Búsqueda**: la clave reportada (`found_key`) debe descifrar un texto que **contenga la frase**.
3. **CSV**: cada ejecución agrega **una fila** con campos consistentes (`timestamp`, `hostname`, `p`, `iterations_done`, etc.).

## 7. Parámetros prácticos y recomendaciones

- **chunk (c)**: esta versión usa $c = 1$.
- **Entrada/salida**:

  - Ejecutar en **WSL dentro de `$HOME`**.
  - No usar `root` con `mpirun`.
  - Garantizar existencia de `data/impl2/` y encabezado único.
- **Repeticiones**: ejecutar ≥3 veces por combinación (`key_upper`, `p`).

## 8. Limitaciones conocidas

- **ECB** filtra patrones (solo con fines didácticos).
- **I/O centralizado** en `rank 0` puede ser cuello de botella.
- **Detección por frase** es un *oracle* simple.

## 9. Posibles mejoras

- **Balanceo dinámico** (*work-stealing*).
- **Comunicación colectiva** (`MPI_Allreduce`).
- **I/O asíncrono** (buffering).
- **Lotes de claves** ($c > 1$).
- **Vectorización** (SIMD) del descifrado.

## 10. Conclusiones

- **Impl2 altA** implementa un patrón **MPI sin maestro** con **parada temprana no bloqueante**.
- La **variabilidad** del punto de hallazgo domina el *speedup*; conviene promediar resultados.
- Con *chunks* mayores y menos sincronización, el **escalamiento** mejora para grandes espacios de claves.

## 11. Referencias (formato APA)

- FIPS PUB 46-3. (1999). *Data Encryption Standard (DES)* (Withdrawn). NIST. [https://csrc.nist.gov/publications/detail/fips/46-3/archive/1999-10-25](https://csrc.nist.gov/publications/detail/fips/46-3/archive/1999-10-25)
- Open MPI Team. (s. f.). *Open MPI Documentation*. [https://www.open-mpi.org/doc/](https://www.open-mpi.org/doc/)
