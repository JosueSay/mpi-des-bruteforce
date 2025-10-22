#!/usr/bin/env bash
# run_par.sh
# Script para automatizar ejecuciones paralelas por bloque (keys × p)
# Uso:
#   ./scripts/run_par.sh -i impl1 -h myhost -m a
#   ./scripts/run_par.sh -i impl1 -h myhost -m m -k 123456 -p 4
#
# TEST_MODE=true -> no ejecuta binarios, solo imprime "pendiente" (por defecto)
# TEST_MODE=false -> ejecuta el binario real
set -u
set -o pipefail

#######################
# Config global
#######################
TEST_MODE=true   # <- Cambiar a false para ejecutar binarios reales

# Colores para salida
CLR_RESET="\e[0m"
CLR_RED="\e[31m"
CLR_GREEN="\e[32m"
CLR_YELLOW="\e[33m"
CLR_BLUE="\e[34m"
CLR_MAGENTA="\e[35m"
CLR_CYAN="\e[36m"
CLR_BOLD="\e[1m"

#######################
# Rutas absolutas
#######################
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
BIN_IMPL1="${SCRIPT_DIR}/../build/bin/impl1" # cambiar nombre ejecutable
BIN_IMPL2="${SCRIPT_DIR}/../build/bin/impl2" # cambiar nombre ejecutable
BIN_IMPL3="${SCRIPT_DIR}/../build/bin/impl3" # cambiar nombre ejecutable

CSV_IMPL1_PAR="${SCRIPT_DIR}/../data/impl1/par.csv"
CSV_IMPL2_PAR="${SCRIPT_DIR}/../data/impl2/par.csv"
CSV_IMPL3_PAR="${SCRIPT_DIR}/../data/impl3/par.csv"

INPUTS_DIR="${SCRIPT_DIR}/../inputs"
FILE_TEXTO="${INPUTS_DIR}/texto_entrada.txt"
FILE_FRASE="${INPUTS_DIR}/frase_busqueda.txt"

#######################
# Llaves (keys) y procesos a probar
#######################
KEYS=( 123456 18014398509481983 18014398509481984 )
P_LIST=( 2 4 8 )   # número de procesos paralelos, ajustable

#######################
# Helper: imprimir uso
#######################
usage() {
  cat <<EOF
Usage: $0 -i <impl1|impl2|impl3> -h <host_name> -m <a|m> [ -k <key> -p <p> ]
  -i impl     Implementation id: impl1, impl2 or impl3
  -h host     Hostname
  -m mode     Mode: 'a' = automatic (itera sobre keys × P_LIST), 'm' = manual (proporciona key y p)
  -k key      (modo manual) la key a probar (integer)
  -p p        (modo manual) número de procesos
  -t          Toggle TEST_MODE a false (ejecuta binarios reales)
  -?          Mostrar ayuda
EOF
  exit 1
}

#######################
# Parse args
#######################
IMPL=""
HOST=""
MODE=""
MANUAL_KEY=""
MANUAL_P=""
while getopts "i:h:m:k:p:t?" opt; do
  case "${opt}" in
    i) IMPL="${OPTARG}" ;;
    h) HOST="${OPTARG}" ;;
    m) MODE="${OPTARG}" ;;
    k) MANUAL_KEY="${OPTARG}" ;;
    p) MANUAL_P="${OPTARG}" ;;
    t) TEST_MODE=false ;;
    ?) usage ;;
  esac
done

# Validaciones básicas
if [[ -z "${IMPL}" || -z "${HOST}" || -z "${MODE}" ]]; then
  echo -e "${CLR_RED}Error: parámetros obligatorios faltantes.${CLR_RESET}"
  usage
fi
if [[ "${MODE}" != "a" && "${MODE}" != "m" ]]; then
  echo -e "${CLR_RED}Error: mode inválido. Use 'a' o 'm'.${CLR_RESET}"
  usage
fi
if [[ "${MODE}" == "m" && ( -z "${MANUAL_KEY}" || -z "${MANUAL_P}" ) ]]; then
  echo -e "${CLR_RED}Error: modo manual requiere -k <key> y -p <p>.${CLR_RESET}"
  usage
fi

# Verificar inputs
if [[ ! -f "${FILE_TEXTO}" ]]; then
  echo -e "${CLR_RED}Error: no existe ${FILE_TEXTO}.${CLR_RESET}"
  exit 2
fi
if [[ ! -f "${FILE_FRASE}" ]]; then
  echo -e "${CLR_RED}Error: no existe ${FILE_FRASE}.${CLR_RESET}"
  exit 2
fi

# Leer contenido de inputs
FRASE=$(tr -d '\r' < "${FILE_FRASE}" | sed -n '1p')
TEXTO=$(tr -d '\r' < "${FILE_TEXTO}")

# Seleccionar paths según impl
case "${IMPL}" in
  impl1)
    BIN_PATH="${BIN_IMPL1}"
    CSV_PAR="${CSV_IMPL1_PAR}"
    ;;
  impl2)
    BIN_PATH="${BIN_IMPL2}"
    CSV_PAR="${CSV_IMPL2_PAR}"
    ;;
  impl3)
    BIN_PATH="${BIN_IMPL3}"
    CSV_PAR="${CSV_IMPL3_PAR}"
    ;;
  *)
    echo -e "${CLR_RED}Error: implementación desconocida '${IMPL}'.${CLR_RESET}"
    exit 3
    ;;
esac

# Mostrar configuración
echo -e "${CLR_CYAN}${CLR_BOLD}\nConfiguración de ejecución:${CLR_RESET}"
echo -e "\tImplem:\t\t${CLR_YELLOW}${IMPL}${CLR_RESET}"
echo -e "\tHost:\t\t${CLR_YELLOW}${HOST}${CLR_RESET}"
echo -e "\tModo:\t\t${CLR_YELLOW}${MODE}${CLR_RESET}"
echo -e "\tTEST_MODE:\t${CLR_YELLOW}${TEST_MODE}${CLR_RESET}"
echo -e "\tBinario:\t${CLR_YELLOW}${BIN_PATH}${CLR_RESET}"
echo -e "\tCSV paralelo:\t${CLR_YELLOW}${CSV_PAR}${CLR_RESET}"
echo -e "\tArchivo frase:\t${CLR_YELLOW}${FILE_FRASE}${CLR_RESET}"
echo -e "\tFrase:\t\t${CLR_YELLOW}${FRASE}${CLR_RESET}"
echo -e "\tTexto:\t\t${CLR_YELLOW}${TEXTO}${CLR_RESET}\n"

#######################
# Función de ejecución paralela
#######################
run_one() {
  local key="$1"
  local p="$2"
  local timestamp
  timestamp=$(date -u +"%Y-%m-%dT%H:%M:%SZ")

  echo -e "${CLR_MAGENTA}${CLR_BOLD}--> Run${CLR_RESET} key=${CLR_GREEN}${key}${CLR_RESET} | p=${CLR_CYAN}${p}${CLR_RESET} | mode=par"
  echo -e "\t${CLR_BOLD}csv:${CLR_RESET} ${CSV_PAR} ${CLR_BOLD}host:${CLR_RESET} ${HOST} ${CLR_BOLD}time:${CLR_RESET} ${timestamp}"

  if [[ "${TEST_MODE}" == true ]]; then
    echo -e "\t${CLR_YELLOW}TEST_MODE=true -> Simulando ejecución:${CLR_RESET}"
    echo -e "\t\techo pendiente  # comando: echo \"${TEXTO}\" | mpirun -np ${p} ${BIN_PATH} \"${FRASE}\" ${key} ${p} \"${CSV_PAR}\" \"${HOST}\"\n"
  else
    echo -e "\t${CLR_GREEN}Ejecutando comando real...${CLR_RESET}"
    echo -e "\t\t${BIN_PATH} con contenido del texto y ${p} procesos"
    echo "${TEXTO}" | mpirun -np ${p} ${BIN_PATH} "${FRASE}" ${key} ${p} "${CSV_PAR}" "${HOST}"
    echo -e "\n"
  fi
}

#######################
# Modo de ejecución
#######################
if [[ "${MODE}" == "a" ]]; then
  for key in "${KEYS[@]}"; do
    for p in "${P_LIST[@]}"; do
      run_one "${key}" "${p}"
    done
  done
elif [[ "${MODE}" == "m" ]]; then
  run_one "${MANUAL_KEY}" "${MANUAL_P}"
fi

echo -e "${CLR_CYAN}${CLR_BOLD}\nEjecuciones finalizadas (o simuladas). Revisa el CSV en ${CSV_PAR}.${CLR_RESET}"
