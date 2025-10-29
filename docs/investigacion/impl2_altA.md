# Implementación 2 (altA) — Investigación técnica

## 1) Modelo criptográfico

### 1.1 DES en modo ECB (con OpenSSL)

* **Bloque** : 64 bits;  **clave efectiva** : 56 bits.
* **Modo** : ECB (Electronic Code Book): cada bloque se cifra de forma independiente.
* **Padding** : convencionalmente múltiplos de 8 bytes (p.ej., PKCS#7).
* **API (vía `core_crypto.h`)** :
* `encryptDesEcb(key56, in, len, &out, &out_len)`
* `decryptDesEcb(key56, in, len, &out, &out_len)`

  Retornan `0` si la operación fue correcta y asignan búfer de salida.

> 1.2 Detección de texto válido

* Se busca una **frase** dentro del texto plano descifrado:
  * `containsPhrase(dec, dec_len, phrase)` → `1` si la contiene, `0` en caso contrario.

---

## 2) Interfaz pública de Impl2 (`impl2.h`)

<pre class="overflow-visible!" data-start="2200" data-end="2599"><div class="contain-inline-size rounded-2xl relative bg-token-sidebar-surface-primary"><div class="sticky top-9"><div class="absolute end-0 bottom-0 flex h-9 items-center pe-2"><div class="bg-token-bg-elevated-secondary text-token-text-secondary flex items-center gap-4 rounded-sm px-2 font-sans text-xs"></div></div></div><div class="overflow-y-auto p-4" dir="ltr"><code class="whitespace-pre! language-c"><span><span>int</span><span></span><span>encryptDesEcb</span><span>(uint64_t</span><span> key56,
                  </span><span>const</span><span></span><span>unsigned</span><span></span><span>char</span><span> *in, </span><span>size_t</span><span> len,
                  </span><span>unsigned</span><span></span><span>char</span><span> **out, </span><span>size_t</span><span> *out_len);

</span><span>int</span><span></span><span>decryptDesEcb</span><span>(uint64_t</span><span> key56,
                  </span><span>const</span><span></span><span>unsigned</span><span></span><span>char</span><span> *in, </span><span>size_t</span><span> len,
                  </span><span>unsigned</span><span></span><span>char</span><span> **out, </span><span>size_t</span><span> *out_len);

</span><span>int</span><span></span><span>containsPhrase</span><span>(const</span><span></span><span>unsigned</span><span></span><span>char</span><span> *buf, </span><span>size_t</span><span> len,
                   </span><span>const</span><span></span><span>char</span><span> *phrase);
</span></span></code></div></div></pre>

* **Contrato común** : el mismo que emplean Impl1/Impl3.
* **Memoria** : `out` es asignado internamente y el  **llamador libera** .

---

## 3) Diseño secuencial (`impl2_seq.c`)

### 3.1 Flujo “encrypt”

1. Lee **stdin** → `plain`.
2. Cifra con `key56` → `cipher`.
3. Escribe binario (`out_bin`) y **agrega una fila** al CSV (`data/impl2/sec.csv`).

### 3.2 Flujo “decrypt/brute”

1. Carga **cifrado** (`in_bin`).
2. Inicia cronómetro (`nowMono()` / `diffMono`).
3. Recorre `k=0..key_upper`:
   * Descifra → `dec`.
   * `containsPhrase(dec, dec_len, phrase)` → *found*
   * Si encuentra:  *break* .
4. Imprime en stdout (`found=…` / `no encontrado`).
5. Escribe fila en CSV:
   * `time_seconds` = tiempo total.
   * `iterations_done` = iteraciones ejecutadas.
   * `found` = 0/1.
   * `p` = valor pasado por CLI (para la tabla; secuencial puede registrar `1` o `0` según convenio interno).

> La escritura en CSV usa utilidades de `common.h`/`core_utils.h`: `ensureHeader`, `isoUtcNow`, `csvSanitize`.

---

## 4) Diseño paralelo MPI (`impl2_par.c`)

### 4.1 Roles y datos compartidos

* **Todos los procesos** participan de la búsqueda (no hay maestro dedicado).
* **Difusión inicial** (según modo):
  * `encrypt`: `rank 0` lee `stdin` y difunde `plain` → cifra → escribe `out_bin` + CSV.
  * `decrypt`: `rank 0` lee `in_bin` y **difunde** `cipher` (tamaño y contenido) a todos.

### 4.2 Esquema de reparto (block-cyclic simple)

Se itera por **turnos** `t = 0,1,2,…` y cada proceso `r` toma claves “intercaladas”:

* **Base por turno** :

  base(t,r)  =  r⋅c  +  t⋅(c⋅P)\text{base}(t, r) \;=\; r \cdot c \;+\; t \cdot (c \cdot P)**base**(**t**,**r**)**=**r**⋅**c**+**t**⋅**(**c**⋅**P**)
  con **chunk** c=1c=1**c**=**1** en esta versión y P=P = **P**= número de procesos.
* **Rango local** : [ base,min⁡(base+c−1,  key_upper) ][\,\text{base}, \min(\text{base}+c-1,\; key\_upper)\,]**[**base**,**min**(**base**+**c**−**1**,**k**ey**_**u**pp**er**)**]**.

> Este patrón reparte uniformemente las claves y  **reduce zonas calientes** ; con `c>1` (extensión futura) amortizas latencias de comunicación.

### 4.3 Parada temprana no bloqueante

* Cualquier proceso que detecta la  **frase** :
  1. Marca `found=1`, `found_key=k`, `winner_rank=r`.
  2. Envía **no bloqueante** `TAG_FOUND` con la clave a todos.
* Todos los procesos **sondean** con `MPI_Iprobe(TAG_FOUND)` antes/durante su lazo:
  * Si llega `TAG_FOUND`, recuperan la **clave ganadora** y salen.

### 6.4 Consolidación de métricas

En la sección final:

* `MPI_Reduce` para:
  * **tiempo** : `MAX` de los locales → `max_time` (marca la pared real del  *wall-clock* ).
  * **iteraciones** : `SUM` → `total_iters`.
  * **found** : `MAX` (0/1) → `found_any`.
  * **winner_rank** : `MAX` (o -1) → `global_winner`.
* `rank 0` imprime un resumen humano y **escribe la fila única** al CSV `data/impl2/par.csv`.

---

## 5) Complejidad y modelo de desempeño

### 5.1 Complejidad básica

* **Secuencial** :

  Ts≈∑k=0key_upper(tdec(k)+tchk(k))      hasta encontrar o agotar.T_s \approx \sum_{k=0}^{key\_upper} (t_{dec}(k) + t_{chk}(k)) \;\;\; \text{hasta encontrar o agotar.}**T**s****≈**k**=**0**∑**k**ey**_**u**pp**er****(**t**d**ec****(**k**)**+**t**c**hk****(**k**))**hasta encontrar o agotar.
* **Paralelo** : idealmente

  Tp≈TsPT_p \approx \frac{T_s}{P}**T**p≈**P**T**s**
  pero en la práctica se ve afectado por:
* **Desequilibrio** (si el hallazgo está muy sesgado al inicio o al final).
* **Overheads** de comunicación (`Bcast`, `Iprobe/Isend`, `Reduce`).
* **I/O CSV** centralizado en `rank 0`.

### 5.2 *Speedup* y eficiencia

* S=TsTp,E=SP.S = \frac{T_s}{T_p}, \qquad E = \frac{S}{P}.**S**=**T**p****T**s******,**E**=**P**S****.
* **Amdahl** : si la fracción secuencial es ff**f**,

  Smax⁡=1f+(1−f)PS_{\max} = \frac{1}{f + \frac{(1-f)}{P}}**S**m**a**x=**f**+**P**(**1**−**f**)****1****
* **Gustafson** (escala el problema con P):

  SG≈P−f⋅(P−1)S_G \approx P - f\cdot (P-1)**S**G≈**P**−**f**⋅**(**P**−**1**)**

> La parada temprana introduce  **variabilidad** : cuando la clave está cerca del inicio, el *speedup* observado puede ser menor (poca *work* útil paralelo); si está cerca del final, la paralelización luce mejor.

---

## 6) Validación de correctitud

1. **Cifrado/Descifrado** : con `-DUSE_OPENSSL` debe producirse un `cipher` que **se recupere** con la  **misma clave** .
2. **Búsqueda** : la clave reportada (`found_key`) realmente debe **descifrar** a un texto que  **contenga la frase** .
3. **CSV** : cada ejecución agrega exactamente **una fila** con campos consistentes (timestamp ISO, `hostname`, `p`, `iterations_done`, etc.).

---

## 7) Parámetros prácticos y recomendaciones

* **chunk (c)** : esta versión usa `c=1`. Si notas *overhead* de comunicación en problemas grandes, incrementa `c` (p.ej., 50k–500k).
* **Entrada/salida** :
* Ejecuta en **WSL dentro de `$HOME`** (evita `/mnt/c/...` para binarios).
* No ejecutes `mpirun` como `root` (usa tu usuario o, si es inevitable, `OMPI_ALLOW_RUN_AS_ROOT=1` y `OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1`).
* **CSV** : garantiza la existencia de `data/impl2/` y **encabezado único** (`ensureHeader`).
* **Repeticiones** : ejecuta **≥3** por combinación (`key_upper`, `p`) para suavizar variaciones.

---

## 8) Limitaciones conocidas

* **ECB** filtra patrones; se usa aquí solo con fines didácticos.
* **I/O centralizado** en `rank 0` puede ser un cuello de botella en ejecuciones masivas.
* **Detección por frase** es un *oracle* simple; casos reales requieren validaciones más robustas (p.ej. diccionarios, scoring de lenguaje).

---

## 9) Posibles mejoras

* **Balanceo dinámico** (work-stealing) para claves “difíciles”.
* **Comunicación colectiva** (por ejemplo, `MPI_Allreduce`) para consolidar flags más rápido.
* **I/O asíncrono** (buffering) para CSV y binarios.
* **Lotes de claves (c > 1)** para amortizar la sincronización.
* **Vectorización** (SIMD) del descifrado si se usa un  *toy cipher* .

---

## 10) Conclusiones

* **Impl2 altA** materializa un patrón **MPI sin maestro** con **parada temprana no bloqueante** que funciona bien para la naturaleza “ *embarrassingly parallel* ” de la fuerza bruta.
* La **variabilidad** del punto de hallazgo domina el *speedup* observado; por ello, conviene reportar métricas agregadas sobre **varias repeticiones** y diferentes posiciones de la clave.
* Con *chunks* mayores y menos sincronización, el **escalamiento** mejora para espacios grandes de claves.

---

## 11) Referencias (formato APA)

* FIPS PUB 46-3. (1999). *Data Encryption Standard (DES)* (Withdrawn). National Institute of Standards and Technology (NIST). [https://csrc.nist.gov/publications/detail/fips/46-3/archive/1999-10-25]()
* Open MPI Team. (s. f.).  *Open MPI Documentation* . Open MPI. [https://www.open-mpi.org/doc/]()
