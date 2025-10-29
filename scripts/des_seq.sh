#!/usr/bin/env bash

set -euo pipefail

# ========= paleta =========
CLR_W=$'\e[97m'; CLR_D=$'\e[90m'; CLR_C=$'\e[36m'; CLR_Y=$'\e[33m'
CLR_G=$'\e[32m'; CLR_M=$'\e[35m'; CLR_B=$'\e[1m'; CLR_RESET=$'\e[0m'

# ========= rutas =========
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
BIN_DIR="${SCRIPT_DIR}/../build/bin"
IO_IN="${SCRIPT_DIR}/../IO/inputs"
IO_OUT="${SCRIPT_DIR}/../IO/outputs"
DATA_DIR="${SCRIPT_DIR}/../data"

BIN_IMPL1="${BIN_DIR}/impl1_seq"
BIN_IMPL2="${BIN_DIR}/impl2_seq"
BIN_IMPL3="${BIN_DIR}/impl3_seq"

CSV1="${DATA_DIR}/impl1/sec.csv"
CSV2="${DATA_DIR}/impl2/sec.csv"
CSV3="${DATA_DIR}/impl3/sec.csv"

PHR_DEF="${IO_IN}/frase_busqueda.txt"

# ========= ayuda =========
usage(){
  local code=${1:-1}
  printf "%b%buso:%b\n" "${CLR_M}" "${CLR_B}" "${CLR_RESET}"
  printf "\t%s -i <impl1|impl2|impl3> -h <host> -K <key_upper> [-f \"frase\"] [-B \"bin1 bin2 ...\"]\n" "$0"
  printf "\n%b%bparámetros:%b\n" "${CLR_M}" "${CLR_B}" "${CLR_RESET}"
  printf "\t-i\timplementación: impl1 | impl2 | impl3\n"
  printf "\t-h\thostname lógico para CSV (ej: myhost)\n"
  printf "\t-K\tkey_upper (máxima llave a probar, entero decimal)\n"
  printf "\t-f\tfrase a buscar (si falta, usa 1ra línea de %s)\n" "${PHR_DEF}"
  printf "\t-B\tlista de .bin (si falta, toma todos en %s)\n" "${IO_OUT}"
  printf "\t-?\tayuda\n\n"
  exit "${code}"
}

# ========= helpers =========
is_uint(){ [[ "$1" =~ ^[0-9]+$ ]]; }
print_hdr(){ printf "%b%bDESCIFRADO SECUENCIAL (DES) — impl=%s host=%s%b\n" "${CLR_M}" "${CLR_B}" "${IMPL}" "${HOST}" "${CLR_RESET}"; }
print_cfg(){
  printf "\n%bConfiguración:%b\n" "${CLR_C}" "${CLR_RESET}"
  printf "\t- %bbinario:%b\t%s\n" "${CLR_B}" "${CLR_RESET}" "${BIN}"
  printf "\t- %bcsv salida:%b\t%s\n" "${CLR_B}" "${CLR_RESET}" "${CSV}"
  printf "\t- %binputs dir:%b\t%s\n" "${CLR_B}" "${CLR_RESET}" "${IO_IN}"
  printf "\t- %boutputs dir:%b\t%s\n" "${CLR_B}" "${CLR_RESET}" "${IO_OUT}"
  printf "\t- %bkey_upper:%b\t%s\n" "${CLR_B}" "${CLR_RESET}" "${KEY_UP}"
  printf "\t- %bfrase:%b\t\"%s\"\n" "${CLR_B}" "${CLR_RESET}" "${PHRASE}"
}

# ========= parse =========
IMPL=""; HOST=""; KEY_UP=""; PHRASE=""; BINS_ARG=""
while getopts "i:h:K:f:B:?" o; do
  case "$o" in
    i) IMPL="$OPTARG" ;; h) HOST="$OPTARG" ;;
    K) KEY_UP="$OPTARG" ;; f) PHRASE="$OPTARG" ;;
    B) BINS_ARG="$OPTARG" ;; ?) usage 0 ;;
  esac
done

[[ -z "${IMPL}" || -z "${HOST}" || -z "${KEY_UP}" ]] && { usage 1; }

# ========= prereqs =========
if ! is_uint "${KEY_UP}"; then echo "-K debe ser entero"; exit 2; fi
case "${IMPL}" in
  impl1) BIN="${BIN_IMPL1}"; CSV="${CSV1}" ;;
  impl2) BIN="${BIN_IMPL2}"; CSV="${CSV2}" ;;
  impl3) BIN="${BIN_IMPL3}"; CSV="${CSV3}" ;;
  *) echo "impl desconocida"; exit 2 ;;
esac

[[ -x "${BIN}" ]] || { echo "no ejecutable: ${BIN}"; exit 2; }
mkdir -p "$(dirname "${CSV}")" "${IO_OUT}"

# ========= frase =========
if [[ -z "${PHRASE}" ]]; then
  [[ -f "${PHR_DEF}" ]] || { echo "no existe ${PHR_DEF}"; exit 3; }
  PHRASE="$(tr -d '\r' < "${PHR_DEF}" | sed -n '1p')"
  [[ -n "${PHRASE}" ]] || { echo "frase vacía"; exit 3; }
fi

# ========= bins =========
declare -a BINS
if [[ -n "${BINS_ARG}" ]]; then
  BINS=(${BINS_ARG})
else
  mapfile -t BINS < <(ls -1 "${IO_OUT}"/*.bin 2>/dev/null || true)
  [[ ${#BINS[@]} -gt 0 ]] || { echo "no hay .bin en ${IO_OUT}"; exit 4; }
fi

# ========= banner =========
print_hdr
print_cfg
printf "\n%bbins a procesar:%b\t%s\n" "${CLR_C}" "${CLR_RESET}" "${#BINS[@]}"
for b in "${BINS[@]}"; do printf "\t- %b%s%b\n" "${CLR_Y}" "${b}" "${CLR_RESET}"; done
printf "\n"

# ========= ejecución =========
found_cnt=0; miss_cnt=0; run_idx=0; total="${#BINS[@]}"
for binf in "${BINS[@]}"; do
  run_idx=$((run_idx + 1))
  [[ -f "${binf}" ]] || { miss_cnt=$((miss_cnt + 1)); continue; }

  printf "%b%b(%d/%d) descencriptar%b\n" "${CLR_M}" "${CLR_B}" "${run_idx}" "${total}" "${CLR_RESET}"
  printf "\t- %bbin:%b\t\t%s\n" "${CLR_B}" "${CLR_RESET}" "${binf}"
  printf "\t- %bfrase:%b\t\"%s\"\n" "${CLR_B}" "${CLR_RESET}" "${PHRASE}"
  printf "\t- %bkey_up:%b\t%s\n" "${CLR_B}" "${CLR_RESET}" "${KEY_UP}"
  printf "\t- %bcsv:%b\t\t%s\n" "${CLR_B}" "${CLR_RESET}" "${CSV}"
  printf "\t- %bhost:%b\t\t%s\n" "${CLR_B}" "${CLR_RESET}" "${HOST}"

  REAL_CMD=( "${BIN}" decrypt "${PHRASE}" "${KEY_UP}" 1 "${CSV}" "${HOST}" "${binf}" )
  printf "%bComando:%b\n\t- %b" "${CLR_C}" "${CLR_RESET}" "${CLR_Y}"; printf "%q " "${REAL_CMD[@]}"; printf "%b\n" "${CLR_RESET}"

  # --- ejecución sin interleaving con spinner controlado por PID ---
  tmpOut=$(mktemp)

  # Ejecuta en background y captura PID
  "${REAL_CMD[@]}" > "${tmpOut}" 2>&1 & CMD_PID=$!

  # Spinner que observa el proceso (solo stdout)
  {
    while kill -0 "${CMD_PID}" 2>/dev/null; do
      printf "\r%b\t… ejecutando …%b" "${CLR_D}" "${CLR_RESET}"
      sleep 0.25
    done
    # limpiar la línea del spinner
    printf "\r%*s\r" 40 ""
  } & SPIN_PID=$!

  # Espera a que termine el comando
  wait "${CMD_PID}"; rc=$?
  # Asegura que el spinner termine
  wait "${SPIN_PID}" 2>/dev/null || true

  if [[ "${rc}" -eq 0 ]] && grep -q "encontrado;" "${tmpOut}"; then
    printf "\t- %bresult:%b\t%bENCONTRADO%b\n\n" "${CLR_B}" "${CLR_RESET}" "${CLR_G}" "${CLR_RESET}"
    found_cnt=$((found_cnt + 1))
  else
    printf "\t- %bresult:%b\t%bNO ENCONTRADO%b (rc=%d)\n\n" "${CLR_B}" "${CLR_RESET}" "${CLR_Y}" "${CLR_RESET}" "${rc}"
    miss_cnt=$((miss_cnt + 1))
  fi

  rm -f "${tmpOut}"
done

# ========= resumen =========
printf "%b%bRESUMEN%b\n" "${CLR_M}" "${CLR_B}" "${CLR_RESET}"
printf "\t- %btotal bins:%b\t\t%s\n" "${CLR_B}" "${CLR_RESET}" "${total}"
printf "\t- %bencontrados:%b\t\t%s\n" "${CLR_B}" "${CLR_RESET}" "${found_cnt}"
printf "\t- %bno encontrados:%b\t%s\n" "${CLR_B}" "${CLR_RESET}" "${miss_cnt}"
printf "\t- %bcsv consolidado:%b\t%s\n" "${CLR_B}" "${CLR_RESET}" "${CSV}"
