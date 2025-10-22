#!/usr/bin/env bash
# run_seq.sh
# Script para automatizar ejecuciones secuenciales por bloque (keys)
# Uso:
#   ./scripts/run_seq.sh -i impl1 -h myhost -m a
#   ./scripts/run_seq.sh -i impl1 -h myhost -m m -k 123456
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

CSV_IMPL1_SEC="${SCRIPT_DIR}/../data/impl1/sec.csv"
CSV_IMPL2_SEC="${SCRIPT_DIR}/../data/impl2/sec.csv"
CSV_IMPL3_SEC="${SCRIPT_DIR}/../data/impl3/sec.csv"

INPUTS_DIR="${SCRIPT_DIR}/../inputs"
FILE_TEXTO="${INPUTS_DIR}/texto_entrada.txt"
FILE_FRASE="${INPUTS_DIR}/frase_busqueda.txt"

#######################
# Llaves (keys) a probar
#######################
KEYS=( \
  123456 \
  18014398509481983 \
  18014398509481984 \
)

#######################
# Helper: imprimir uso
#######################
usage() {
  cat <<EOF
Usage: $0 -i <impl1|impl2|impl3> -h <host_name> -m <a|m> [ -k <key> ]
  -i impl     Implementation id: impl1, impl2 or impl3
  -h host     Hostname (identificador de la máquina que ejecuta)
  -m mode     Mode: 'a' = automatic (itera sobre keys), 'm' = manual (proporciona key)
  -k key      (modo manual) la key a probar (integer)
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
while getopts "i:h:m:k:t?" opt; do
  case "${opt}" in
    i) IMPL="${OPTARG}" ;;
    h) HOST="${OPTARG}" ;;
    m) MODE="${OPTARG}" ;;
    k) MANUAL_KEY="${OPTARG}" ;;
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
if [[ "${MODE}" == "m" && -z "${MANUAL_KEY}" ]]; then
  echo -e "${CLR_RED}Error: modo manual requiere -k <key>.${CLR_RESET}"
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
    CSV_SEC="${CSV_IMPL1_SEC}"
    ;;
  impl2)
    BIN_PATH="${BIN_IMPL2}"
    CSV_SEC="${CSV_IMPL2_SEC}"
    ;;
  impl3)
    BIN_PATH="${BIN_IMPL3}"
    CSV_SEC="${CSV_IMPL3_SEC}"
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
echo -e "\tCSV secuencial:\t${CLR_YELLOW}${CSV_SEC}${CLR_RESET}"
echo -e "\tArchivo frase:\t${CLR_YELLOW}${FILE_FRASE}${CLR_RESET}"
echo -e "\tFrase:\t\t${CLR_YELLOW}${FRASE}${CLR_RESET}"
echo -e "\tTexto:\t\t${CLR_YELLOW}${TEXTO}${CLR_RESET}\n"

#######################
# Función de ejecución secuencial
#######################
run_one() {
  local key="$1"
  local timestamp
  timestamp=$(date -u +"%Y-%m-%dT%H:%M:%SZ")

  echo -e "${CLR_MAGENTA}${CLR_BOLD}--> Run${CLR_RESET} key=${CLR_GREEN}${key}${CLR_RESET} | mode=sec"
  echo -e "\t${CLR_BOLD}csv:${CLR_RESET} ${CSV_SEC} ${CLR_BOLD}host:${CLR_RESET} ${HOST} ${CLR_BOLD}time:${CLR_RESET} ${timestamp}"

  # Enviar contenido directamente al binario
  if [[ "${TEST_MODE}" == true ]]; then
    echo -e "\t${CLR_YELLOW}TEST_MODE=true -> Simulando ejecución:${CLR_RESET}"
    echo -e "\t\techo pendiente  # comando: echo \"${TEXTO}\" | ${BIN_PATH} \"${FRASE}\" ${key} 1 \"${CSV_SEC}\" \"${HOST}\"\n"
  else
    echo -e "\t${CLR_GREEN}Ejecutando comando real...${CLR_RESET}"
    echo -e "\t\t${BIN_PATH} con contenido del texto"
    echo "${TEXTO}" | ${BIN_PATH} "${FRASE}" ${key} 1 "${CSV_SEC}" "${HOST}"
    echo -e "\n"
  fi
}

#######################
# Modo de ejecución
#######################
if [[ "${MODE}" == "a" ]]; then
  for key in "${KEYS[@]}"; do
    run_one "${key}"
  done
elif [[ "${MODE}" == "m" ]]; then
  run_one "${MANUAL_KEY}"
fi

echo -e "${CLR_CYAN}${CLR_BOLD}\nEjecuciones finalizadas (o simuladas). Revisa el CSV en ${CSV_SEC}.${CLR_RESET}"
