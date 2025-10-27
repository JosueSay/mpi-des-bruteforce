# 🔐 Proyecto 2 – Descifrado de llaves privadas con MPI

Descripcion

## 📂 Estructura del proyecto

- Arbol del proyecto

## 📦 Requisitos e Instalación

**(Ubuntu 22.04 / WSL):**

```bash
sudo apt update
sudo apt install -y build-essential pkg-config libssl-dev openssl openmpi-bin libopenmpi-dev dos2unix
dos2unix scripts/*.sh
chmod +x scripts/*.sh
gcc -v
lsb_release -a
```

## ⚙️ Compilación

```bash
# todas las secuenciales
make all-seq USE_OPENSSL=1

# todas las paralelas (MPI)
make all-par USE_OPENSSL=1

# una específica
make impl1-seq USE_OPENSSL=1
make impl1-par USE_OPENSSL=1

# limpiar
make clean
```

Genera `build/bin/impl1`, `build/bin/impl2`, `build/bin/impl3`.

## 🚀 Ejecución con scripts

Los scripts soportan **automático** (usa archivos `inputs/`) y **manual** (recibe parámetros explícitos).

Parámetros comunes de los scripts:

- `-i` → implementación (`impl1`|`impl2`|`impl3`)
- `-h` → hostname
- `-m` → modo `a` (auto) / `m` (manual)
- `-k` → llave (manual)
- `-p` → procesos (solo run_par)
- `-f` → frase (manual)
- `-x` → texto (manual)
- `-?` → ayuda

> Los scripts leen por defecto `inputs/texto_entrada.txt` y `inputs/frase_busqueda.txt` en modo automático.

### Secuencial

**Automático:**

```bash
./scripts/run_seq.sh -i impl1 -h myhost -m a
```

**Manual (key OR key+frase+texto):**

```bash
# mínimo: llave
./scripts/run_seq.sh -i impl1 -h myhost -m m -k 123456

# con frase y texto explícitos
./scripts/run_seq.sh -i impl1 -h myhost -m m -k 123456 -f "es una prueba de" -x "Esta es una prueba de proyecto 2"
```

**Ayuda:**

```bash
./scripts/run_seq.sh -?
```

### Paralelo (MPI)

**Automático:**

```bash
./scripts/run_par.sh -i impl1 -h myhost -m a
```

**Manual:**

```bash
# mínimo: llave y procesos
./scripts/run_par.sh -i impl1 -h myhost -m m -k 123456 -p 4

# con frase y texto explícitos
./scripts/run_par.sh -i impl1 -h myhost -m m -k 123456 -p 4 \
  -f "es una prueba de" -x "Esta es una prueba de proyecto 2"
```

**Ayuda:**

```bash
./scripts/run_par.sh -?
```

## 📊 Salida (CSV)

Encabezado (ambos):

```csv
implementation,key,p,repetition,time_seconds,iterations_done,found,finder_rank,timestamp,hostname,phrase,text
```

- Secuencial → `data/impl1/sec.csv`
- Paralelo → `data/impl1/par.csv`
