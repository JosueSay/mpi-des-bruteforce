# 🔐 Proyecto 2 – Descifrado de llaves privadas con MPI

Descripcion

## 📂 Estructura del proyecto

- Arbol del proyecto

Entendí que quieres el README actualizado **exactamente** según los scripts que implementaste (sin añadir secciones nuevas ni usar `;` ni `echo`), reflejando las opciones reales de cada `*.sh` (por eso hay diferencias con ejemplos previos). Abajo va el README con los comandos y opciones ajustados a los scripts actuales.  

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

# una específica (ejemplo impl1)
make impl1-seq USE_OPENSSL=1
make impl1-par USE_OPENSSL=1

# limpiar build
make clean
```

Genera `build/bin/impl1_seq`, `build/bin/impl1_par`, `build/bin/impl2_seq`, `build/bin/impl2_par`, `build/bin/impl3_seq`, `build/bin/impl3_par`.

## 🚀 Ejecución con scripts

Los scripts soportan dos flujos pero **cada script tiene sus flags propios**; usar exactamente las opciones que acepta el script.

Parámetros comunes (según script)

- `-i` implementación `impl1` | `impl2` | `impl3`
- `-h` hostname lógico para CSV
- `-k` llave (key) usada para cifrar en los scripts de encriptado
- `-K` key_upper (límite) usado en los scripts de desencriptado/crack secuencial
- `-x` texto inline para cifrar
- `-X` archivo con múltiples líneas (lote)
- `-f` frase para buscar en el descifrado/crack
- `-B` lista de `.bin` (solo en `des_seq.sh`)
- `-?` ayuda

> En modo por defecto los scripts usan los archivos en `IO/inputs` (`texto_entrada.txt` y `frase_busqueda.txt`) según corresponda. Revisa cada ayuda de script si dudas.

### Secuencial — Encriptar (encr_seq.sh)

Entrada: texto por stdin, `-x` texto inline o `-X` archivo con líneas (lote)

**Automático (usa `IO/inputs/texto_entrada.txt` si no hay stdin ni `-x`):**

```bash
./scripts/encr_seq.sh -i impl1 -h myhost -k 123456
```

**Manuales:**

texto inline

```bash
./scripts/encr_seq.sh -i impl1 -h myhost -k 123456 -x "Esta es una prueba de proyecto 2"
```

lote desde archivo

```bash
./scripts/encr_seq.sh -i impl1 -h myhost -k 123456 -X IO/inputs/lote.txt
```

**Ayuda:**

```bash
./scripts/encr_seq.sh -?
```

### Secuencial — Desencriptar / Crack (des_seq.sh)

Este script espera `-K` como límite superior de llave y procesa uno o varios `.bin` en `IO/outputs` o los listados con `-B`. Usa la frase con `-f` o la primera línea de `IO/inputs/frase_busqueda.txt`.

**Automático (procesa todos los `.bin` en `IO/outputs` y usa frase por defecto):**

```bash
./scripts/des_seq.sh -i impl1 -h myhost -K 18014398509481984
```

**Manual (frase explícita y lista de .bin):**

```bash
./scripts/des_seq.sh -i impl1 -h myhost -K 2000000 -f "es una prueba" -B "IO/outputs/cipher_A.bin IO/outputs/cipher_B.bin"
```

**Ayuda:**

```bash
./scripts/des_seq.sh -?
```

### Paralelo (MPI) — Encriptar (encr_par.sh)

Entrada: texto por stdin, `-x` texto inline o `-X` archivo con líneas (lote)

**Automático (usa `IO/inputs/texto_entrada.txt` si no hay stdin ni `-x`):**

```bash
./scripts/encr_par.sh -i impl1 -h myhost -k 123456
```

**Manual (texto inline):**

```bash
./scripts/encr_par.sh -i impl1 -h myhost -k 123456 -x "Esta es una prueba de proyecto 2"
```

**Lote desde archivo:**

```bash
./scripts/encr_par.sh -i impl1 -h myhost -k 123456 -X IO/inputs/lote.txt
```

**Ayuda:**

```bash
./scripts/encr_par.sh -?
```

Referencia del comportamiento y opciones de `encr_par.sh`.

### Paralelo (MPI) — Desencriptar / Crack (des_par.sh)

Uso equivalente a `des_seq.sh` pero pensado para flujo paralelo; revisa `des_par.sh` para parámetros exactos y ejemplos incluidos en su ayuda.

**Automático:**

```bash
./scripts/des_par.sh -i impl1 -h myhost -K 18014398509481984
```

**Manual (ejemplo con frase y archivo para generar cipher si es necesario):**

```bash
./scripts/des_par.sh -i impl1 -h myhost -K 2000000 -f "es una prueba" -I IO/outputs/cipher.bin
```

**Ayuda:**

```bash
./scripts/des_par.sh -?
```

## 📊 Salida (CSV)

Encabezado usado por los binarios y los scripts:

```csv
implementation,mode,key,p,repetition,time_seconds,iterations_done,found,finder_rank,timestamp,hostname,phrase,text,out_bin
```

Rutas CSV por implementación

- Secuencial → `data/impl1/sec.csv`, `data/impl2/sec.csv`, `data/impl3/sec.csv`
- Paralelo → `data/impl1/par.csv`, `data/impl2/par.csv`, `data/impl3/par.csv`
