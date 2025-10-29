#!/usr/bin/env bash

set -euo pipefail

# ===== paleta (white/dark + acentos) =====
CLR_W=$'\e[97m'   # bright white (etiquetas principales / fondo claro)
CLR_D=$'\e[90m'   # dark grey (mensajes neutrales)
CLR_C=$'\e[36m'   # cyan (valores)
CLR_Y=$'\e[33m'   # yellow (comandos)
CLR_G=$'\e[32m'   # green (rutas / ok)
CLR_M=$'\e[35m'   # magenta (resultados / títulos)
CLR_B=$'\e[1m'    # bold
CLR_R=$'\e[31m'   # red (errores)
CLR_RESET=$'\e[0m'
trap 'printf "%b" "${CLR_RESET}"' EXIT

# ===== rutas =====
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
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

TXT_DEF="${IO_IN}/texto_entrada.txt"
LOTE_DEF="${IO_IN}/lote.txt"

# ===== ayuda =====
usage(){
  local code=${1:-1}
  printf "%buso:%b\n" "${CLR_M}${CLR_B}" "${CLR_RESET}"
  printf "\t- echo \"texto\" | %s -i <impl1|impl2|impl3> -h <host> -k <key>\n" "$0"
  printf "\t- %s -i impl1 -h <host> -k <key>\t# usa %s si no hay stdin ni -x\n" "$0" "${TXT_DEF}"
  printf "\t- %s -i impl1 -h <host> -k <key> -x \"texto directo\"\n" "$0"
  printf "\t- %s -i impl1 -h <host> -k <key> -X %s\t# cada línea => un .bin\n\n" "$0" "${LOTE_DEF}"

  printf "%bparámetros:%b\n" "${CLR_M}${CLR_B}" "${CLR_RESET}"
  printf "\t-i\timplementación: impl1 | impl2 | impl3\n"
  printf "\t-h\thostname lógico para CSV (ej: myhost)\n"
  printf "\t-k\tllave real (key_true) para cifrar (entero decimal)\n"
  printf "\t-x\ttexto directo a cifrar (opcional)\n"
  printf "\t-X\tarchivo con múltiples líneas a cifrar (opcional)\n"
  printf "\t-?\tayuda\n\n"

  printf "%bsalida:%b\n" "${CLR_M}${CLR_B}" "${CLR_RESET}"
  printf "\t- Archivos:\t%s/cipher_<YYYYMMDDTHHMMSSZ>_<PID>.bin\n" "${IO_OUT}"
  printf "\t- CSV:\t\tdata/implX/par.csv con mode=encrypt\n"
  exit "${code}"
}

# ===== util =====
is_uint(){ [[ "$1" =~ ^[0-9]+$ ]]; }
uniq_bin(){ printf "%s/cipher_%s_%d.bin" "${IO_OUT}" "$(date -u +%Y%m%dT%H%M%SZ)" "$$"; }

# $1:texto, escribe bin y guarda en GENERATED
encrypt_text(){
  local t="$1"
  local out
  out="$(uniq_bin)"

  if [[ -z "${t}" ]]; then
    printf "%b\t%s\t%s%b\n" "${CLR_D}" "skipping" "(texto vacío)" "${CLR_RESET}"
    return 0
  fi

  # encabezado de operación
  printf "%b\t- %s%b\n" "${CLR_W}${CLR_B}" "encrypt(par)" "${CLR_RESET}"
  printf "\t- %bkey:%b\t%b%s%b\n" "${CLR_B}" "${CLR_RESET}" "${CLR_W}" "${KEY_TRUE}" "${CLR_RESET}"

  # comando real
  printf "%bComando:%b\n" "${CLR_C}" "${CLR_RESET}"
  printf "\t- %bmpirun -np 1 %q encrypt %q %q %q %q%b\n" \
    "${CLR_Y}" "${BIN}" "${KEY_TRUE}" "${out}" "${CSV}" "${HOST}" "${CLR_RESET}"

  # resultados (origen / destino)
  printf "%bResultados:%b\n" "${CLR_C}" "${CLR_RESET}"
  printf "\t- %borigen:%b\t%s\n" "${CLR_B}" "${CLR_RESET}" "${SRC_LABEL}"
  printf "\t- %boutput-file:%b\t%s\n" "${CLR_B}" "${CLR_RESET}" "${out}"

  # ejecutar y capturar stdout/stderr
  local tmpOut tmpErr rc
  tmpOut=$(mktemp) || exit 1
  tmpErr=$(mktemp) || { rm -f "${tmpOut}"; exit 1; }
  rc=0
  printf '%s' "${t}" | mpirun -np 1 "${BIN}" encrypt "${KEY_TRUE}" "${out}" "${CSV}" "${HOST}" >"${tmpOut}" 2>"${tmpErr}" || rc=$?

  # mostrar stdout/stderr agrupados
  if [[ -s "${tmpOut}" ]]; then
    printf "\t%bstdout:%b\n" "${CLR_W}" "${CLR_RESET}"
    sed -e 's/^/\t\t/' "${tmpOut}" | sed -e "s/^/${CLR_D}/" -e "s/$/${CLR_RESET}/"
  fi
  if [[ -s "${tmpErr}" ]]; then
    printf "\t%bstderr:%b\n" "${CLR_W}" "${CLR_RESET}"
    sed -e 's/^/\t\t/' "${tmpErr}" | sed -e "s/^/${CLR_R}/" -e "s/$/${CLR_RESET}/"
  fi

  rm -f "${tmpOut}" "${tmpErr}"

  if [[ ${rc} -ne 0 || ! -s "${out}" ]]; then
    printf "\t- %bresult:%b\t%bFAILED (rc=%d)%b\n\n" "${CLR_B}" "${CLR_RESET}" "${CLR_R}" "${rc}" "${CLR_RESET}"
    return 1
  fi

  printf "\t- %bresult:%b\t%bOK%b\n\n" "${CLR_B}" "${CLR_RESET}" "${CLR_G}" "${CLR_RESET}"
  GENERATED+=( "${out}" )
  return 0
}

# ===== parse args =====
IMPL=""; HOST=""; KEY_TRUE=""; TEXT_ONE=""; TEXT_LIST=""
while getopts "i:h:k:x:X:?" o; do
  case "$o" in
    i) IMPL="$OPTARG";;
    h) HOST="$OPTARG";;
    k) KEY_TRUE="$OPTARG";;
    x) TEXT_ONE="$OPTARG";;
    X) TEXT_LIST="$OPTARG";;
    ?) usage 0;;
  esac
done

if [[ -z "${IMPL}" || -z "${HOST}" || -z "${KEY_TRUE}" ]]; then
  printf "%b\terror:%b\tfaltan parámetros obligatorios (usa -? para ayuda)\n\n" "${CLR_R}" "${CLR_RESET}"
  usage 1
fi

is_uint "${KEY_TRUE}" || { printf "%b\t-k (key) debe ser entero decimal%b\n" "${CLR_R}" "${CLR_RESET}"; exit 2; }

case "$IMPL" in
  impl1) BIN="${BIN_IMPL1}"; CSV="${CSV1}";;
  impl2) BIN="${BIN_IMPL2}"; CSV="${CSV2}";;
  impl3) BIN="${BIN_IMPL3}"; CSV="${CSV3}";;
  *) printf "%b\timpl desconocida (impl1|impl2|impl3)%b\n" "${CLR_R}" "${CLR_RESET}"; exit 2;;
esac
[[ -x "${BIN}" ]] || { printf "%b\tno existe ejecutable: %s%b\n" "${CLR_R}" "${BIN}" "${CLR_RESET}"; exit 2; }
mkdir -p "${IO_OUT}" "$(dirname "${CSV}")"

# ===== banner =====
printf "%b%s%b\n" "${CLR_M}${CLR_B}" "CIFRADO PARALELO (DES) — impl=${IMPL} host=${HOST}" "${CLR_RESET}"

printf "\n%bConfiguración:%b\n" "${CLR_C}" "${CLR_RESET}"
printf "\t- %bbinario:%b\t%s\n" "${CLR_B}" "${CLR_RESET}" "${BIN}"
printf "\t- %bcsv salida:%b\t%s\n" "${CLR_B}" "${CLR_RESET}" "${CSV}"
printf "\t- %boutputs dir:%b\t%s\n" "${CLR_B}" "${CLR_RESET}" "${IO_OUT}"
printf "\t- %bkey_true:%b\t%s\n" "${CLR_B}" "${CLR_RESET}" "${KEY_TRUE}"

# ===== origen de texto =====
declare -a GENERATED=()
SRC_LABEL=""
if [[ -n "${TEXT_LIST}" ]]; then
  [[ -f "${TEXT_LIST}" ]] || { printf "%b\terror: no existe %s%b\n" "${CLR_R}" "${TEXT_LIST}" "${CLR_RESET}"; exit 4; }
  MODE="lote"; SRC_LABEL="${TEXT_LIST}"
elif ! [ -t 0 ]; then
  MODE="stdin"; SRC_LABEL="stdin"
elif [[ -n "${TEXT_ONE}" ]]; then
  MODE="inline"; SRC_LABEL="inline"
else
  [[ -f "${TXT_DEF}" ]] || { printf "%b\terror: no existe %s y no se pasó -x/-X ni stdin%b\n" "${CLR_R}" "${TXT_DEF}" "${CLR_RESET}"; exit 5; }
  MODE="archivo"; SRC_LABEL="${TXT_DEF}"
fi
printf "\t- %bmodo entrada:%b\t%s\n\n" "${CLR_B}" "${CLR_RESET}" "${MODE}"

# ===== ejecución =====
case "${MODE}" in
  lote)
    idx=0; ok=0; fail=0
    while IFS= read -r line || [[ -n "$line" ]]; do
      ((idx++))
      txt="${line%$'\r'}"
      if [[ -z "${txt}" ]]; then
        printf "\t%s\t%s\n" "${CLR_D}(l${idx})" "línea vacía, se omite"
        continue
      fi
      printf "\t%s\t%s\n" "${CLR_M}(l${idx})" "cifrando línea..."
      if encrypt_text "${txt}"; then ((ok++)); else ((fail++)); fi
    done < "${TEXT_LIST}"
    printf "\n\t%bresumen lote:%b\tok=%s\tfail=%s\n\n" "${CLR_B}${CLR_W}" "${CLR_RESET}" "${ok}" "${fail}"
    ;;
  stdin)
    TEXT_STDIN="$(cat -)"
    encrypt_text "${TEXT_STDIN}"
    ;;
  inline)
    encrypt_text "${TEXT_ONE}"
    ;;
  archivo)
    encrypt_text "$(cat "${TXT_DEF}")"
    ;;
esac

# ===== resumen final =====
count="${#GENERATED[@]}"
printf "%b%s%b\n" "${CLR_M}${CLR_B}" "RESUMEN" "${CLR_RESET}"
printf "\t- %bbinarios generados:%b\t%s\n" "${CLR_B}" "${CLR_RESET}" "${count}"
for b in "${GENERATED[@]}"; do
  printf "\t\t- %b%s%b\n" "${CLR_W}" "${b}" "${CLR_RESET}"
done
printf "\t- %bcsv modificado:%b\n\t\t- %s\n" "${CLR_B}" "${CLR_RESET}" "${CSV}"
