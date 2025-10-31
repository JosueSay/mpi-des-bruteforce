# 🔐 Proyecto 2 – Descifrado de llaves privadas con MPI

Implementación de tres aproximaciones para romper claves DES por **fuerza bruta** (secuencial y paralela con Open MPI).

Incluye scripts de ejecución, generación de `.bin`, recolección de métricas en CSV y análisis de speedup.

Los cálculos y análisis numéricos se simplificaron usando **Python**: en `math/` hay un *Jupyter notebook* (`math.ipynb`) que genera las tablas/estadísticas y las gráficas que se documentan en `reports/`.

## 📂 Estructura del proyecto

```bash
.
├── IO
│   ├── inputs
│   │   ├── frase_busqueda.txt
│   │   └── texto_entrada.txt
│   └── outputs
│       └── cipher_<timestamp>.bin
├── Makefile               # genera build/ con binarios y objetos
├── data/                  # CSV de resultados (impl1/impl2/impl3 / par|sec)
├── docs/                  # documentación y guía
├── images/                # diagramas y evidencia
├── include/               # headers
├── math/                  # notebook y cálculos (math.ipynb)
├── reports/               # reporte final y anexos (usa outputs de math/)
├── scripts/               # encr_/des_ *.sh (ejecución)
└── src/                   # código fuente (impl1/2/3 *_seq/_par)
```

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

1. Limpiar los binarios y objetos:

    ```bash
    make clean
    ```

2. Compilar todos los archivos (secuenciales/paralelos):

    ```bash
    make all-seq USE_OPENSSL=1
    make all-par USE_OPENSSL=1
    ```

> Opcional si solo deseas correr los de una implementación en específica puedes ejecutar estos comandos (puede ser impl1, impl2 o impl3):
>
> ```bash
> make impl1-seq USE_OPENSSL=1
> make impl1-par USE_OPENSSL=1
>```

Genera: `build/bin/impl{1,2,3}_{seq,par}`.

## 🚀 Ejecución con scripts

Los scripts tienen flags propios. Usa exactamente los que acepta cada uno.

### Encriptar

No importa cual implementación se utilice, lo importante es tener un archivo `.bin` sobre la encriptación realizada, para probar los diferentes enfoques de descifrado.

Utilizar el parámetro `-?` en los `.sh` para saber sobre los parámetros disponibles y descripción.

#### Secuencial - (`encr_seq.sh`)

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

#### Paralelo - (`encr_par.sh`)

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

### Descencriptar

#### Secuencial - (`des_seq.sh`)

```bash
# automático (todos los .bin y frase por defecto)
./scripts/des_seq.sh -i impl1 -h myhost -K 18014398509481984

# manual (frase y bins específicos)
./scripts/des_seq.sh -i impl1 -h myhost -K 2000000 -f "es una prueba de" -B "IO/outputs/cipher_A.bin IO/outputs/cipher_B.bin"

# ayuda
./scripts/des_seq.sh -?
```

#### Paralelo - (`des_par.sh`)

```bash
# automático
./scripts/des_par.sh -i impl1 -h myhost -n 8 -K 18014398509481984

# manual
./scripts/des_par.sh -i impl1 -h myhost -n 8 -K 2000000 -f "es una prueba de" -B "IO/outputs/cipher.bin"

# ayuda
./scripts/des_par.sh -?
```

## 💻 Comandos utilizados

```bash
# Cifrar
./scripts/encr_seq.sh -i impl1 -h myhost -k 123456


# Descifrado secuencial - implementación 1
./scripts/des_seq.sh -i impl1 -h myhost -K 123456
./scripts/des_seq.sh -i impl1 -h myhost -K 18014398509481983
./scripts/des_seq.sh -i impl1 -h myhost -K 72057594037927935


# Descifrado secuencial - implementación 2

./scripts/des_seq.sh -i impl2 -h myhost -K 123456
./scripts/des_seq.sh -i impl2 -h myhost -K 18014398509481983
./scripts/des_seq.sh -i impl2 -h myhost -K 72057594037927935

# Descifrado secuencial - implementación 3

./scripts/des_seq.sh -i impl3 -h myhost -K 123456
./scripts/des_seq.sh -i impl3 -h myhost -K 18014398509481983
./scripts/des_seq.sh -i impl3 -h myhost -K 72057594037927935


# Descifrado paralelo - implementación 1

./scripts/des_par.sh -i impl1 -h myhost -n 2 -K 123456
./scripts/des_par.sh -i impl1 -h myhost -n 2 -K 18014398509481983
./scripts/des_par.sh -i impl1 -h myhost -n 2 -K 72057594037927935


./scripts/des_par.sh -i impl1 -h myhost -n 4 -K 123456
./scripts/des_par.sh -i impl1 -h myhost -n 4 -K 18014398509481983
./scripts/des_par.sh -i impl1 -h myhost -n 4 -K 72057594037927935

./scripts/des_par.sh -i impl1 -h myhost -n 8 -K 123456
./scripts/des_par.sh -i impl1 -h myhost -n 8 -K 18014398509481983
./scripts/des_par.sh -i impl1 -h myhost -n 8 -K 72057594037927935


# Descifrado paralelo - implementación 2

./scripts/des_par.sh -i impl2 -h myhost -n 2 -K 123456
./scripts/des_par.sh -i impl2 -h myhost -n 2 -K 18014398509481983
./scripts/des_par.sh -i impl2 -h myhost -n 2 -K 72057594037927935

./scripts/des_par.sh -i impl2 -h myhost -n 4 -K 123456
./scripts/des_par.sh -i impl2 -h myhost -n 4 -K 18014398509481983
./scripts/des_par.sh -i impl2 -h myhost -n 4 -K 72057594037927935

./scripts/des_par.sh -i impl2 -h myhost -n 8 -K 123456
./scripts/des_par.sh -i impl2 -h myhost -n 8 -K 18014398509481983
./scripts/des_par.sh -i impl2 -h myhost -n 8 -K 72057594037927935

# Descifrado paralelo - implementación 3

./scripts/des_par.sh -i impl3 -h myhost -n 2 -K 123456
./scripts/des_par.sh -i impl3 -h myhost -n 2 -K 18014398509481983
./scripts/des_par.sh -i impl3 -h myhost -n 2 -K 72057594037927935


./scripts/des_par.sh -i impl3 -h myhost -n 4 -K 123456
./scripts/des_par.sh -i impl3 -h myhost -n 4 -K 18014398509481983
./scripts/des_par.sh -i impl3 -h myhost -n 4 -K 72057594037927935

./scripts/des_par.sh -i impl3 -h myhost -n 8 -K 123456
./scripts/des_par.sh -i impl3 -h myhost -n 8 -K 18014398509481983
./scripts/des_par.sh -i impl3 -h myhost -n 8 -K 72057594037927935
```

## 📊 Salida (CSV)

Encabezado:

```csv
implementation,mode,key,p,repetition,time_seconds,iterations_done,found,finder_rank,timestamp,hostname,phrase,text,out_bin
```

Rutas:

- Secuencial → `data/impl{1,2,3}/sec.csv`
- Paralelo   → `data/impl{1,2,3}/par.csv`
