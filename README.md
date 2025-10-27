# 🔐 Proyecto 2 – Descifrado de llaves privadas con MPI

Descripcion

## 📂 Estructura del proyecto

- Arbol del proyecto

## 📦 Requisitos e Instalación

### Instalación (Ubuntu 22.04 / WSL)

```bash
# 1) actualizar sistema
sudo apt update

# 2) toolchain básico (incluye gcc y make)
sudo apt install -y build-essential pkg-config

# 3) crypto para DES real (OpenSSL)
sudo apt install -y libssl-dev openssl

# 4) mpi para la versión paralela
sudo apt install -y openmpi-bin libopenmpi-dev

# 5) utilidades
sudo apt install -y dos2unix

# 6) verificar versiones
gcc -v
lsb_release -a
```

Normaliza finales de línea y permisos de scripts:

```bash
dos2unix scripts/*.sh
chmod +x scripts/*.sh
```

### Compilación

Por defecto, los artefactos van a `build/bin` (ejecutables) y `build/obj` (objetos).

```bash
# sólo secuenciales (impl1/impl2/impl3)
make all-seq USE_OPENSSL=1

# sólo paralelos (impl1/impl2/impl3) — requiere OpenMPI
make all-par USE_OPENSSL=1

# uno específico
make impl1-seq USE_OPENSSL=1
make impl1-par USE_OPENSSL=1

# limpiar
make clean
```

**Se generan:**

- `build/bin/impl1`
- `build/bin/impl2`
- `build/bin/impl3`

### Ejecución con scripts

Se tienen scripts que soportan **modo automático** (lista de llaves y/o procesos predefinidos) y **modo manual**.
Por defecto `TEST_MODE=true` (simula). Para ejecutar de verdad, pasa `-t` (lo pone en false).

> Reemplaza `impl1` por `impl2` o `impl3` para las otras implementaciones.

#### Secuencial

```bash
# automático (usa KEYS del script)
./scripts/run_seq.sh -i impl1 -h myhost -m a -t

# manual (key específica)
./scripts/run_seq.sh -i impl1 -h myhost -m m -k 123456 -t
```

**Qué hace:**

- Lee `inputs/texto_entrada.txt` (stdin para el binario) y `inputs/frase_busqueda.txt`.
- Ejecuta `build/bin/impl1` (secuencial) y apendea resultados en `data/impl1/sec.csv`.

#### Paralelo

```bash
# automático (combina KEYS × P_LIST)
./scripts/run_par.sh -i impl1 -h myhost -m a -t

# manual (key y procesos específicos)
./scripts/run_par.sh -i impl1 -h myhost -m m -k 123456 -p 4 -t
```

**Qué hace:**

- usa `mpirun -np <p> build/bin/impl1` y apendea en `data/impl1/par.csv`.

### Ejecución directa (sin scripts)

```bash
# ejemplo secuencial (impl1)
echo "texto de prueba para des" | build/bin/impl1 "frase a buscar" 123456 1 "data/impl1/sec.csv" "myhost"
```

**argumentos del binario (stdin → texto):**

```bash
<frase> <key_upper_bound> <p> <ruta_csv> <hostname>
```

### Salida de resultados (CSV)

Encabezado (secuencial y paralelo):

```csv
implementation,key,p,repetition,time_seconds,iterations_done,found,finder_rank,timestamp,hostname
```

- secuencial escribe en `data/impl1/sec.csv`
- paralelo escribe en `data/impl1/par.csv`

## ⚙️ Compilación y ejecución

- Cómo compilar los binarios (`make` o comando `gcc/mpicc`).
- Scripts de ejecución disponibles:
  - `run_seq.sh` → versión secuencial
  - `run_par.sh` → versión paralela
- Parámetros que aceptan los scripts:
  - `-i` → implementación (`impl1`, `impl2`, `impl3`)
  - `-h` → host (nombre de la máquina)
  - `-m` → modo (`a` automático, `m` manual)
  - `-k` → llave (solo en modo manual)
  - `-p` → número de procesos (solo para versión paralela)
  - `-t` → toggle TEST_MODE (`true` = simulación, `false` = ejecución real)
- Ejemplos de uso:

  ```bash
  ./scripts/run_seq.sh -i impl1 -h myhost -m a
  ./scripts/run_par.sh -i impl1 -h myhost -m m -k 123456 -p 4
  ```

- Explicación de los resultados y CSV generado:

  - Campos: `implementation,key,p,repetition,time_seconds,iterations_done,found,finder_rank,timestamp,hostname`
