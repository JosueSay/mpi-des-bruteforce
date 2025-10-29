# Configuración de Entradas y Scripts

Este documento describe los **parámetros de entrada** que deben recibir las implementaciones y **cómo ejecutar los scripts** provistos.

## Ubicación de los archivos de entrada

Los archivos están en `IO/inputs/`:

```bash
IO/inputs/
├── texto_entrada.txt       # Texto en claro usado para cifrar / referencia para pruebas
└── frase_busqueda.txt      # Palabra o frase clave (una línea) para validar descifrado
```

El binario/los scripts usan `IO/outputs/` para los `.bin` generados/consumidos.

## Parámetros de entrada

1. **Texto de entrada (`texto_entrada.txt`)**

   - Descripción: texto plano que se cifra para generar `.bin` (o se usa como stdin).
   - Ruta: `IO/inputs/texto_entrada.txt`.
   - Uso: leído por los scripts si no se pasa `-x` ni stdin.

2. **Palabra / frase clave (`frase_busqueda.txt`)**

   - Descripción: substring esperado en el texto descifrado para confirmar la llave.
   - Ruta: `IO/inputs/frase_busqueda.txt` (primera línea).
   - Uso: pasado como argumento al binario si no se indica `-f`.

3. **Llave objetivo / key_upper**

   - Descripción: llave real (para `encrypt`) o límite superior de búsqueda (para `decrypt`).
   - Paso: `-k` (encrypt) o `-K` (decrypt) en los scripts.
   - Valor: entero decimal (ej. `123456`, `18014398509481983`, `18014398509481984`).

4. **Número de procesos (`p`)**

   - Descripción: número de procesos MPI.
   - Ejecución: `mpirun -np <p>` dentro de los scripts paralelos (`des_par.sh`, `encr_par.sh`).
   - Nota: en paralelo `p` controla la partición del espacio; en secuencial `p=1`.

## Interfaz general de los binarios

- `stdin` : texto completo (cuando aplica).
- `argv[1]` : frase (decrypt) o (encrypt) key (según modo).
- `argv[2]` : key / key_upper.
- `argv[3]` : p (en secuencial se pasa `1`; en paralelo se pasa `p`).
- `argv[4]` : ruta al CSV de salida.
- `argv[5]` : hostname lógico para registrar.
- (par) `argv[6]` : input `.bin` (para decrypt).

> Los scripts ya construyen y pasan estos argumentos; no necesitas invocar el binario manualmente salvo para depuración.

## Scripts disponibles (resumen y ejemplos)

Rutas: `./scripts/{encr_seq.sh,encr_par.sh,des_seq.sh,des_par.sh}`

### `encr_seq.sh` — cifrado **secuencial**

- Usa `texto_entrada.txt` o `-x "<texto>"` / `-X <archivo>` (lote).
- Parámetros mínimos: `-i <impl1|impl2|impl3> -h <host> -k <key>`
- Ejemplo:

```bash
./scripts/encr_seq.sh -i impl1 -h myhost -k 123456
./scripts/encr_seq.sh -i impl2 -h myhost -k 42 -x "Esta es una prueba"
```

### `encr_par.sh` — cifrado **paralelo** (MPI)

- Igual comportamiento pero ejecutado con `mpirun -np 1` internamente (encrypt suele usar 1).
- Parámetros: `-i -h -k` y opcionales `-x -X`.
- Ejemplo:

```bash
./scripts/encr_par.sh -i impl1 -h myhost -n 4 -k 123456
```

### `des_seq.sh` — descifrado **secuencial**

- Lee `.bin` desde `IO/outputs/*.bin` (o `-B` lista).
- Parámetros: `-i <impl> -h <host> -K <key_upper> [-f "<frase>"] [-B "<bin1 bin2 ...>"]`
- Ejemplos:

```bash
./scripts/des_seq.sh -i impl1 -h myhost -K 123456
./scripts/des_seq.sh -i impl2 -h myhost -K 18014398509481983 -f "es una prueba de"
```

### `des_par.sh` — descifrado **paralelo (MPI)**

- Ejecuta `mpirun -np <NP>` internamente; itera sobre `.bin` o la lista `-B`.
- Parámetros: `-i <impl> -h <host> -n <np> -K <key_upper> [-f "<frase>"] [-B "<bin1 bin2 ...>"]`
- Ejemplos (pruebas recomendadas):

```bash
# impl1
./scripts/des_par.sh -i impl1 -h myhost -n 2 -K 123456
./scripts/des_par.sh -i impl1 -h myhost -n 4 -K 18014398509481983
./scripts/des_par.sh -i impl1 -h myhost -n 8 -K 18014398509481984

# impl2
./scripts/des_par.sh -i impl2 -h myhost -n 2 -K 123456
./scripts/des_par.sh -i impl2 -h myhost -n 4 -K 18014398509481983
./scripts/des_par.sh -i impl2 -h myhost -n 8 -K 18014398509481984

# impl3
./scripts/des_par.sh -i impl3 -h myhost -n 2 -K 123456
./scripts/des_par.sh -i impl3 -h myhost -n 4 -K 18014398509481983
./scripts/des_par.sh -i impl3 -h myhost -n 8 -K 18014398509481984
```

## CSV de salida y métricas

Encabezado (global):

```csv
implementation,mode,key,p,repetition,time_seconds,iterations_done,found,finder_rank,timestamp,hostname,phrase,text,out_bin
```

Rutas por implementación:

- Secuencial → `data/impl{1,2,3}/sec.csv`
- Paralelo   → `data/impl{1,2,3}/par.csv`
