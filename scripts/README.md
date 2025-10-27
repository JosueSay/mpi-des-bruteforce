# Configuración de Entradas y Scripts

Este documento describe los **parámetros de entrada que todas las implementaciones del proyecto deben recibir y ejecución de scripts**.

## Ubicación de los archivos de entrada

Los archivos de entrada se encuentran en la carpeta `inputs/`:

```bash
inputs/
├── texto_entrada.txt       # Contiene el texto cifrado que se desea descifrar
└── frase_busqueda.txt      # Contiene la palabra o frase clave a buscar en el texto descifrado
```

## Parámetros de entrada

### 1. Texto de entrada (`texto_entrada.txt`)

* **Descripción:** Texto cifrado que el programa intentará descifrar usando fuerza bruta.
* **Ubicación:** `inputs/texto_entrada.txt`
* **Tipo:** Archivo de texto plano (`.txt`)
* **Valor fijo:** Sí. Este archivo debe mantenerse igual para todas las pruebas obligatorias.
* **Uso:** Cargado por el programa al iniciar; todas las implementaciones usan el mismo texto.

### 2. Palabra o frase clave (`frase_busqueda.txt`)

* **Descripción:** Substring que debe aparecer en el texto descifrado si se encuentra la llave correcta.
* **Ubicación:** `inputs/frase_busqueda.txt`
* **Tipo:** Archivo de texto plano (`.txt`)
* **Valor fijo:** Sí. No debe cambiar entre ejecuciones.
* **Uso:** Permite validar que el descifrado fue exitoso (por ejemplo, usando `strstr` o función equivalente).

### 3. Llave objetivo

* **Descripción:** La llave privada utilizada para cifrar el texto o el rango de llaves a explorar.
* **Ubicación:** Definida en el script de ejecución (`.sh`) o como parámetro al ejecutar el programa.
* **Tipo:** Número entero grande (por ejemplo, `123456` o llaves “fácil”, “media”, “difícil”).
* **Valor fijo:** No. Cambia entre pruebas para evaluar performance y speedup.
* **Uso:** Cada ejecución probará distintas llaves o rangos de llaves según la estrategia definida.

### 4. Número de procesos (`p`)

* **Descripción:** Cantidad de procesos paralelos a utilizar en la ejecución (solo aplica para versiones paralelas).
* **Ubicación:** Definido en el script de ejecución (`mpirun -np p`).
* **Tipo:** Entero positivo (`1, 2, 4, 8, ...`)
* **Valor fijo:** No. Puede variar para estudiar escalabilidad y eficiencia.
* **Uso:** Divide el espacio de búsqueda entre procesos; controlado por `mpirun` en la ejecución paralela.

## Instrucciones de uso de los scripts (`run_seq.sh` y `run_par.sh`)

A continuación se documenta **qué hace cada script**, **qué parámetros recibe** y **los comandos válidos** para ejecutarlos.

### `run_seq.sh` — ejecución **secuencial** (por clave)

**Propósito:** ejecutar la implementación secuencial sobre una o varias llaves, recoger métricas y escribir una línea en el CSV secuencial correspondiente.

**Qué hace:**

* Lee `inputs/texto_entrada.txt` y envía su contenido al ejecutable por `stdin`.
* Lee `inputs/frase_busqueda.txt` (primera línea) y la pasa como argumento.
* En modo manual, también puede recibir frase (`-f`) y texto (`-x`) directamente desde consola.
* Para cada `key` ejecuta el binario una vez (por defecto, sin repeticiones adicionales).
* El binario debe recibir los argumentos: `frase key p csv_path host` (en secuencial `p` = 1).
* Guarda o actualiza resultados en el CSV secuencial (`data/implX/sec.csv`).
* Por defecto, el script **ejecuta realmente el binario** (`TEST_MODE=false`).

**Parámetros del script:**

* `-i <impl1|impl2|impl3>` → implementación.
* `-h <host_name>` → nombre del host (se guarda en CSV).
* `-m <a|m>` → modo de ejecución:

  * `a` = automático (itera sobre todas las llaves del array `KEYS`).
  * `m` = manual (usa una sola key indicada con `-k`).
* `-k <key>` → (modo `m`) llave a probar.
* `-f "<frase>"` → (opcional, modo `m`) frase de búsqueda personalizada.
* `-x "<texto>"` → (opcional, modo `m`) texto de entrada personalizado.
* `-?` → muestra ayuda.

**Interfaz del ejecutable:**

* `stdin` → texto completo.
* `argv[1]` → `frase`.
* `argv[2]` → `key`.
* `argv[3]` → `p` (en secuencial siempre `1`).
* `argv[4]` → `csv_path`.
* `argv[5]` → `hostname`.

**Comandos válidos (ejemplos):**

* Automático (usa archivos `inputs/`):

  ```bash
  ./scripts/run_seq.sh -i impl1 -h myhost -m a
  ```

* Manual (solo con key):

  ```bash
  ./scripts/run_seq.sh -i impl1 -h myhost -m m -k 123456
  ```

* Manual con frase y texto personalizados:

  ```bash
  ./scripts/run_seq.sh -i impl1 -h myhost -m m -k 123456 \
    -f "es una prueba de" -x "Esta es una prueba de proyecto 2"
  ```

### `run_par.sh` — ejecución **paralela** (keys × p)

**Propósito:** ejecutar la implementación paralela sobre combinaciones de llaves y número de procesos `p`, recoger métricas y escribir en el CSV paralelo.

**Qué hace:**

* Lee `inputs/texto_entrada.txt` (o usa `-x` en manual) y lo pasa a todos los procesos MPI por `stdin`.
* Lee `inputs/frase_busqueda.txt` (o usa `-f` en manual) y lo pasa como argumento.
* En modo automático (`-m a`) itera sobre todas las `KEYS` y valores de `P_LIST`.
* En modo manual (`-m m`) ejecuta una sola combinación `-k <key>` y `-p <p>`.
* Ejecuta con `mpirun -np <p> <bin>`.
* Guarda resultados en `data/implX/par.csv`.
* Por defecto, el script **ejecuta realmente el binario** (`TEST_MODE=false`).

**Parámetros del script:**

* `-i <impl1|impl2|impl3>` → implementación.
* `-h <host_name>` → nombre del host.
* `-m <a|m>` → modo de ejecución.
* `-k <key>` → (modo `m`) llave a probar.
* `-p <p>` → (modo `m`) número de procesos.
* `-f "<frase>"` → (opcional, modo `m`) frase de búsqueda.
* `-x "<texto>"` → (opcional, modo `m`) texto a procesar.
* `-?` → muestra ayuda.

**Interfaz del ejecutable:**

* `stdin` → texto completo.
* `argv[1]` → `frase`.
* `argv[2]` → `key`.
* `argv[3]` → `p`.
* `argv[4]` → `csv_path`.
* `argv[5]` → `hostname`.

**Comandos válidos (ejemplos):**

* Automático (itera keys × P_LIST):

  ```bash
  ./scripts/run_par.sh -i impl1 -h myhost -m a
  ```

* Manual (key y procesos específicos):

  ```bash
  ./scripts/run_par.sh -i impl1 -h myhost -m m -k 123456 -p 4
  ```

* Manual con frase y texto personalizados:

  ```bash
  ./scripts/run_par.sh -i impl1 -h myhost -m m -k 123456 -p 4 \
    -f "es una prueba de" -x "Esta es una prueba de proyecto 2"
  ```

**Notas para paralela:**

* Asegúrate de que `mpirun` funcione correctamente antes de ejecutar.
* No lances múltiples ejecuciones paralelas simultáneas en la misma máquina.
* Ajusta `P_LIST` según tu hardware (`2,4,8,...`).
