#!/usr/bin/env bash
set -euo pipefail

# ========= paleta =========
CLR_W=$'\e[97m'; CLR_D=$'\e[90m'; CLR_C=$'\e[36m'; CLR_Y=$'\e[33m'
CLR_G=$'\e[32m'; CLR_M=$'\e[35m'; CLR_B=$'\e[1m'; CLR_R=$'\e[31m'; CLR_RESET=$'\e[0m'
trap 'printf "%b" "${CLR_RESET}"' EXIT

# ========= rutas =========
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
BIN_DIR="${SCRIPT_DIR}/../build/bin"
IO_IN="${SCRIPT_DIR}/../IO/inputs"
IO_OUT="${SCRIPT_DIR}/../IO/outputs"
DATA_DIR="${SCRIPT_DIR}/../data"

BIN_IMPL1="${BIN_DIR}/impl1_par"
BIN_IMPL2="${BIN_DIR}/impl2_par"
BIN_IMPL3="${BIN_DIR}/impl3_par"

CSV1="${DATA_DIR}/impl1/par.csv"
CSV2="${DATA_DIR}/impl2/par.csv"
CSV3="${DATA_DIR}/impl3/par.csv"

PHR_DEF="${IO_IN}/frase_busqueda.txt"

# ========= ayuda =========
usage(){
  local code=${1:-1}
  printf "%b%buso:%b\n" "${CLR_M}" "${CLR_B}" "${CLR_RESET}"
  printf "\t%s -i <impl1|impl2|impl3> -h <host> -n <np> -K <key_upper> [-f \"frase\"] [-B \"bin1 bin2 ...\"]\n" "$0"
  printf "\n%b%bparámetros:%b\n" "${CLR_M}" "${CLR_B}" "${CLR_RESET}"
  printf "\t-i\timplementación: impl1 | impl2 | impl3\n"
  printf "\t-h\thostname lógico para CSV (ej: myhost)\n"
  printf "\t-n\tnúmero de procesos MPI (>=1)\n"
  printf "\t-K\tkey_upper (máxima llave a probar, entero decimal)\n"
  printf "\t-f\tfrase a buscar (si falta, usa 1ra línea de %s)\n" "${PHR_DEF}"
  printf "\t-B\tlista de .bin (si falta, toma todos en %s)\n" "${IO_OUT}"
  printf "\n%b%bejemplos:%b\n" "${CLR_M}" "${CLR_B}" "${CLR_RESET}"
  printf "\t%s -i impl1 -h guate-node -n 8 -K 18014398509481984\n" "$0"
  printf "\t%s -i impl1 -h guate-node -n 8 -K 2000000 -f \"es una prueba\" -B \"%s/c1.bin %s/c2.bin\"\n" "$0" "${IO_OUT}" "${IO_OUT}"
  exit "${code}"
}

# ========= helpers =========
require_cmd(){ command -v "$1" >/dev/null 2>&1 || { printf "%b\terror:%b\tfalta comando requerido: %s\n" "${CLR_R}" "${CLR_RESET}" "$1"; exit 2; }; }
is_uint(){ [[ "$1" =~ ^[0-9]+$ ]]; }
print_hdr(){ printf "%b%bDESCIFRADO PARALELO (DES) — impl=%s host=%s%b\n" "${CLR_M}" "${CLR_B}" "${IMPL}" "${HOST}" "${CLR_RESET}"; }
print_cfg(){
  printf "\n%bConfiguración:%b\n" "${CLR_C}" "${CLR_RESET}"
  printf "\t- %bbinario:%b\t%s\n"     "${CLR_B}" "${CLR_RESET}" "${BIN}"
  printf "\t- %bcsv salida:%b\t%s\n"  "${CLR_B}" "${CLR_RESET}" "${CSV}"
  printf "\t- %binputs dir:%b\t%s\n"  "${CLR_B}" "${CLR_RESET}" "${IO_IN}"
  printf "\t- %boutputs dir:%b\t%s\n" "${CLR_B}" "${CLR_RESET}" "${IO_OUT}"
  printf "\t- %bnp:%b\t\t%s\n"        "${CLR_B}" "${CLR_RESET}" "${NP}"
  printf "\t- %bkey_upper:%b\t%s\n"   "${CLR_B}" "${CLR_RESET}" "${KEY_UP}"
  printf "\t- %bfrase:%b\t\t\"%s\"\n" "${CLR_B}" "${CLR_RESET}" "${PHRASE}"
}

# ========= parse =========
IMPL=""; HOST=""; NP=""; KEY_UP=""; PHRASE=""; BINS_ARG=""
while getopts "i:h:n:K:f:B:?" o; do
  case "$o" in
    i) IMPL="$OPTARG" ;;  h) HOST="$OPTARG" ;;
    n) NP="$OPTARG"  ;;  K) KEY_UP="$OPTARG" ;;
    f) PHRASE="$OPTARG" ;; B) BINS_ARG="$OPTARG" ;;
    ?) usage 0 ;;
  esac
done
[[ -z "${IMPL}" || -z "${HOST}" || -z "${NP}" || -z "${KEY_UP}" ]] && { printf "%b\terror:%b\tfaltan parámetros (usa -?)\n" "${CLR_R}" "${CLR_RESET}"; usage 1; }

# ========= prereqs =========
require_cmd mpirun; require_cmd sed; require_cmd tr; require_cmd grep
is_uint "${NP}" || { printf "%b\terror:%b\t-n debe ser entero >=1\n" "${CLR_R}" "${CLR_RESET}"; exit 2; }
[[ "${NP}" -ge 1 ]] || { printf "%b\terror:%b\t-n debe ser entero >=1\n" "${CLR_R}" "${CLR_RESET}"; exit 2; }
is_uint "${KEY_UP}" || { printf "%b\terror:%b\t-K (key_upper) debe ser entero decimal\n" "${CLR_R}" "${CLR_RESET}"; exit 2; }

case "${IMPL}" in
  impl1) BIN="${BIN_IMPL1}"; CSV="${CSV1}" ;;
  impl2) BIN="${BIN_IMPL2}"; CSV="${CSV2}" ;;
  impl3) BIN="${BIN_IMPL3}"; CSV="${CSV3}" ;;
  *) printf "%b\terror:%b\timpl desconocida (impl1|impl2|impl3)\n" "${CLR_R}" "${CLR_RESET}"; exit 2 ;;
esac
[[ -x "${BIN}" ]] || { printf "%b\terror:%b\tno existe ejecutable: %s\n" "${CLR_R}" "${CLR_RESET}" "${BIN}"; exit 2; }
mkdir -p "$(dirname "${CSV}")" "${IO_OUT}"

# ========= frase =========
if [[ -z "${PHRASE}" ]]; then
  [[ -f "${PHR_DEF}" ]] || { printf "%b\terror:%b\tno existe %s y no se pasó -f\n" "${CLR_R}" "${CLR_RESET}" "${PHR_DEF}"; exit 3; }
  PHRASE="$(tr -d '\r' < "${PHR_DEF}" | sed -n '1p')"
  [[ -n "${PHRASE}" ]] || { printf "%b\terror:%b\tfrase vacía en %s\n" "${CLR_R}" "${CLR_RESET}" "${PHR_DEF}"; exit 3; }
fi

# ========= bins =========
declare -a BINS
if [[ -n "${BINS_ARG}" ]]; then
  # shellcheck disable=SC2206
  BINS=(${BINS_ARG})
else
  mapfile -t BINS < <(ls -1 "${IO_OUT}"/*.bin 2>/dev/null || true)
  [[ ${#BINS[@]} -gt 0 ]] || { printf "%b\terror:%b\tno hay .bin en %s (usa -B)\n" "${CLR_R}" "${CLR_RESET}" "${IO_OUT}"; exit 4; }
fi

# ========= banner =========
print_hdr
print_cfg
printf "\n%bbins a procesar:%b\t%s\n" "${CLR_C}" "${CLR_RESET}" "${#BINS[@]}"
for b in "${BINS[@]}"; do printf "\t- %b%s%b\n" "${CLR_G}" "${b}" "${CLR_RESET}"; done
printf "\n"

# ========= ejecución =========
found_cnt=0; miss_cnt=0; idx=0; total="${#BINS[@]}"

for binf in "${BINS[@]}"; do
  idx=$((idx + 1))
  if [[ ! -f "${binf}" ]]; then
    printf "%b\t(%d/%d) skip:%b\tno existe %s\n\n" "${CLR_Y}" "${idx}" "${total}" "${CLR_RESET}" "${binf}"
    miss_cnt=$((miss_cnt + 1)); continue
  fi

  printf "%b%b(%d/%d) descencriptar%b\n" "${CLR_M}" "${CLR_B}" "${idx}" "${total}" "${CLR_RESET}"
  printf "\t- %bbin:%b\t\t%s\n"     "${CLR_B}" "${CLR_RESET}" "${binf}"
  printf "\t- %bfrase:%b\t\"%s\"\n" "${CLR_B}" "${CLR_RESET}" "${PHRASE}"
  printf "\t- %bnp:%b\t\t%s\n"      "${CLR_B}" "${CLR_RESET}" "${NP}"
  printf "\t- %bkey_up:%b\t%s\n"  "${CLR_B}" "${CLR_RESET}" "${KEY_UP}"
  printf "\t- %bcsv:%b\t\t%s\n"     "${CLR_B}" "${CLR_RESET}" "${CSV}"
  printf "\t- %bhost:%b\t\t%s\n"    "${CLR_B}" "${CLR_RESET}" "${HOST}"

  REAL_CMD=( mpirun -np "${NP}" "${BIN}" decrypt "${PHRASE}" "${KEY_UP}" "${NP}" "${CSV}" "${HOST}" "${binf}" )
  printf "%bComando:%b\n\t- %b" "${CLR_C}" "${CLR_RESET}" "${CLR_Y}"; printf "%q " "${REAL_CMD[@]}"; printf "%b\n" "${CLR_RESET}"

  # --- ejecutar con spinner controlado por PID, sin re-ejecutar ---
  tmpOut=$(mktemp)

  # Ejecuta en background, captura PID
  "${REAL_CMD[@]}" > "${tmpOut}" 2>&1 & CMD_PID=$!

  # Spinner vigilando el proceso
  {
    while kill -0 "${CMD_PID}" 2>/dev/null; do
      printf "\r%b\t… ejecutando …%b" "${CLR_D}" "${CLR_RESET}"
      sleep 0.25
    done
    printf "\r%*s\r" 40 ""
  } & SPIN_PID=$!

  # Espera final y cierra spinner
  wait "${CMD_PID}"; rc=$?
  wait "${SPIN_PID}" 2>/dev/null || true

  # Mostrar salida (resumida y limpia)
  if [[ -s "${tmpOut}" ]]; then
    printf "\t%bstdout:%b\n" "${CLR_W}" "${CLR_RESET}"
    sed -e 's/^/\t\t/' "${tmpOut}" | sed -e "s/^/${CLR_D}/" -e "s/$/${CLR_RESET}/"
  fi

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
