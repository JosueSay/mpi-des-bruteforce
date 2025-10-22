# 🔐 Proyecto 2 – Descifrado de llaves privadas con MPI

Descripcion

## 📂 Estructura del proyecto

- Arbol del proyecto

## 📦 Requisitos e Instalación

- Dependencias necesarias: compilador C/C++, Open MPI.
- Comandos de instalación en Linux/Mac.
- Versiones, ambientes, etc.

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
