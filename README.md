# 🔐 Proyecto 2 – Descifrado de llaves privadas con MPI

Descripcion

## 📂 Estructura del proyecto

- Arbol del proyecto

## 📦 Requisitos e instalación (Ubuntu 22.04 / WSL)

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
make impl1-par  USE_OPENSSL=1

# limpiar
make clean
```

Genera: `build/bin/impl{1,2,3}_{seq,par}`.

## 🚀 Ejecución con scripts

Los scripts tienen flags propios. Usa exactamente los que acepta cada uno.

### Secuencial — Encriptar (`encr_seq.sh`)

Entrada: stdin, `-x "<texto>"` o `-X <archivo>`.

```bash
# automático (toma IO/inputs/texto_entrada.txt)
./scripts/encr_seq.sh -i impl1 -h myhost -k 123456

# texto inline
./scripts/encr_seq.sh -i impl1 -h myhost -k 123456 -x "Esta es una prueba de proyecto 2"

# lote desde archivo
./scripts/encr_seq.sh -i impl1 -h myhost -k 123456 -X IO/inputs/lote.txt

# ayuda
./scripts/encr_seq.sh -?
```

### Secuencial — Desencriptar / Crack (`des_seq.sh`)

Procesa `.bin` en `IO/outputs` o los pasados con `-B`. Usa `-f` o `IO/inputs/frase_busqueda.txt`.

```bash
# automático (todos los .bin y frase por defecto)
./scripts/des_seq.sh -i impl1 -h myhost -K 18014398509481984

# manual (frase y bins específicos)
./scripts/des_seq.sh -i impl1 -h myhost -K 2000000 -f "es una prueba de" -B "IO/outputs/cipher_A.bin IO/outputs/cipher_B.bin"

# ayuda
./scripts/des_seq.sh -?
```

### Paralelo (MPI) — Encriptar (`encr_par.sh`)

Entrada: stdin, `-x "<texto>"` o `-X <archivo>`. Requiere `-n`.

```bash
# automático
./scripts/encr_par.sh -i impl1 -h myhost -n 4 -k 123456

# texto inline
./scripts/encr_par.sh -i impl1 -h myhost -n 8 -k 123456 -x "Esta es una prueba de proyecto 2"

# lote desde archivo
./scripts/encr_par.sh -i impl1 -h myhost -n 8 -k 123456 -X IO/inputs/lote.txt

# ayuda
./scripts/encr_par.sh -?
```

### Paralelo (MPI) — Desencriptar / Crack (`des_par.sh`)

Procesa `.bin` en `IO/outputs` o con `-B`. Usa `-f` o `IO/inputs/frase_busqueda.txt`. Requiere `-n`.

```bash
# automático
./scripts/des_par.sh -i impl1 -h myhost -n 8 -K 18014398509481984

# manual
./scripts/des_par.sh -i impl1 -h myhost -n 8 -K 2000000 -f "es una prueba de" -B "IO/outputs/cipher.bin"

# ayuda
./scripts/des_par.sh -?
```

## 📊 Salida (CSV)

Encabezado:

```csv
implementation,mode,key,p,repetition,time_seconds,iterations_done,found,finder_rank,timestamp,hostname,phrase,text,out_bin
```

Rutas:

- Secuencial → `data/impl{1,2,3}/sec.csv`
- Paralelo   → `data/impl{1,2,3}/par.csv`
