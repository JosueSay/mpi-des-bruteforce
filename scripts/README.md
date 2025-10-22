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

- **Descripción:** Texto cifrado que el programa intentará descifrar usando fuerza bruta.
- **Ubicación:** `inputs/texto_entrada.txt`
- **Tipo:** Archivo de texto plano (`.txt`)
- **Valor fijo:** Sí. Este archivo debe mantenerse igual para todas las pruebas obligatorias.
- **Uso:** Cargado por el programa al iniciar; todas las implementaciones usan el mismo texto.

### 2. Palabra o frase clave (`frase_busqueda.txt`)

- **Descripción:** Substring que debe aparecer en el texto descifrado si se encuentra la llave correcta.
- **Ubicación:** `inputs/frase_busqueda.txt`
- **Tipo:** Archivo de texto plano (`.txt`)
- **Valor fijo:** Sí. No debe cambiar entre ejecuciones.
- **Uso:** Permite validar que el descifrado fue exitoso (por ejemplo, usando `strstr` o función equivalente).

### 3. Llave objetivo

- **Descripción:** La llave privada utilizada para cifrar el texto o el rango de llaves a explorar.
- **Ubicación:** Definida en el script de ejecución (`.sh`) o como parámetro al ejecutar el programa.
- **Tipo:** Número entero grande (por ejemplo, `123456` o aproximaciones de llaves "fácil", "media", "difícil").
- **Valor fijo:** No. Cambia entre pruebas para evaluar performance y speedup.
- **Uso:** Cada ejecución probará distintas llaves o rangos de llaves según la estrategia definida.

### 4. Número de procesos (`p`)

- **Descripción:** Cantidad de procesos paralelos a utilizar en la ejecución (solo aplica para versiones paralelas).
- **Ubicación:** Definido en el script de ejecución (`mpirun -np p`).
- **Tipo:** Entero positivo (`1, 2, 4, 8, ...`)
- **Valor fijo:** No. Puede variar para estudiar escalabilidad y eficiencia.
- **Uso:** Divide el espacio de búsqueda entre procesos; controlado por `mpirun` en la ejecución paralela.

## Instrucciones de uso de los scripts (`run_seq.sh` y `run_par.sh`)

A continuación se documenta **qué hace cada script**, **qué parámetros recibe** y **los comandos válidos** para ejecutarlos (incluyendo el flag `-t` que habilita la ejecución real).

### `run_seq.sh` — ejecución **secuencial** (por clave)

**Propósito:** ejecutar la implementación secuencial sobre una o varias llaves, recoger métricas y escribir una línea en el CSV secuencial correspondiente.

**Qué hace:**

- Lee `inputs/texto_entrada.txt` y envía su contenido al ejecutable por `stdin`.
- Lee `inputs/frase_busqueda.txt` (primera línea) y la pasa como argumento.
- Para cada `key` ejecuta el binario una vez (por defecto, sin repeticiones adicionales).
- El binario debe recibir los argumentos: `frase key p csv_path host` (en secuencial `p` = 1).
- Guarda/actualiza resultados en el CSV secuencial definido para la implementación (`data/implX/sec.csv`).
- Por defecto el script está en `TEST_MODE=true` (simula; no ejecuta el binario).

**Parámetros del script:**

- `-i <impl1|impl2|impl3>`: identifica la implementación (elige binario y CSV).
- `-h <host_name>`: nombre identificador de la máquina (se escribe en CSV).
- `-m <a|m>`: modo:
  - `a` = automático (itera sobre todas las llaves definidas en el array `KEYS`).
  - `m` = manual (ejecuta sólo la llave pasada con `-k`).
- `-k <key>`: (modo `m`) llave a probar (entera).
- `-t`: toggle — si se incluye pone `TEST_MODE=false` y **ejecuta** los binarios; si se omite el script sólo simula.

**Interfaz que recibe el ejecutable (requerimiento):**

- `stdin` → contenido completo de `texto_entrada.txt`.
- `argv[1]` → `frase` (primera línea de `frase_busqueda.txt`).
- `argv[2]` → `key`.
- `argv[3]` → `p` (en secuencial siempre `1`).
- `argv[4]` → `csv_path` (ruta del CSV secuencial).
- `argv[5]` → `hostname`.

**Comandos válidos (ejemplos):**

- Simulación automática:

  ```bash
  ./scripts/run_seq.sh -i impl1 -h myhost -m a
  ```

- Ejecutar realmente (no simulado), automático:

  ```bash
  ./scripts/run_seq.sh -i impl1 -h myhost -m a -t
  ```

- Simulación manual (una key):

  ```bash
  ./scripts/run_seq.sh -i impl1 -h myhost -m m -k 123456
  ```

- Ejecutar manual real (no simulado):

  ```bash
  ./scripts/run_seq.sh -i impl1 -h myhost -m m -k 123456 -t
  ```

### `run_par.sh` — ejecución **paralela** (keys × p)

**Propósito:** ejecutar la implementación paralela sobre combinaciones de llaves y número de procesos `p`, recoger métricas y actualizar el CSV paralelo correspondiente.

**Qué hace:**

- Lee `inputs/texto_entrada.txt` y envía su contenido por `stdin` al proceso MPI (cada `mpirun` recibe el stream por stdin).
- Lee `inputs/frase_busqueda.txt` (primera línea) y la pasa como argumento.
- En modo automático (`-m a`) itera sobre todas las `KEYS` y sobre `P_LIST` (lista de valores `p` definida en el script).
- En modo manual (`-m m`) ejecuta una sola combinación `-k <key>` y `-p <p>`.
- Lanza la ejecución usando `mpirun -np <p> <bin>` y pasa los argumentos al binario.
- Guarda/actualiza resultados en el CSV paralelo (`data/implX/par.csv`).
- Por defecto está en `TEST_MODE=true` (simulación).

**Parámetros del script:**

- `-i <impl1|impl2|impl3>`: implementación.
- `-h <host_name>`: nombre del host.
- `-m <a|m>`: modo `a` (automático) o `m` (manual).
- `-k <key>`: (modo `m`) llave a probar.
- `-p <p>`: (modo `m`) número de procesos a usar con `mpirun`.
- `-t`: toggle → realmente ejecuta (si se incluye `TEST_MODE=false`).

**Interfaz que recibe el ejecutable (requerimiento):**

- `stdin` → contenido de `texto_entrada.txt`.
- `argv[1]` → `frase`.
- `argv[2]` → `key`.
- `argv[3]` → `p` (número de procesos).
- `argv[4]` → `csv_path` (ruta del CSV paralelo).
- `argv[5]` → `hostname`.

**Comandos válidos (ejemplos):**

- Simulación automática (itera keys × P_LIST):

  ```bash
  ./scripts/run_par.sh -i impl1 -h myhost -m a
  ```

- Ejecutar automático real (no simulado):

  ```bash
  ./scripts/run_par.sh -i impl1 -h myhost -m a -t
  ```

- Simulación manual (una combinación):

  ```bash
  ./scripts/run_par.sh -i impl1 -h myhost -m m -k 123456 -p 4
  ```

- Ejecutar manual real:

  ```bash
  ./scripts/run_par.sh -i impl1 -h myhost -m m -k 123456 -p 4 -t
  ```

**Notas para paralela:**

- Asegúrate de que `mpirun` esté instalado y funcione en tu entorno.
- No lanzar múltiples ejecuciones paralelas simultáneas en la misma máquina (pueden interferir y generar tiempos no representativos).
- `P_LIST` (valores por defecto) suele incluir `2,4,8` — ajusta según tu hardware.
