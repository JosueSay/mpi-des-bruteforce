# =========================
# Proyecto DES bruteforce
# compila ejecutables impl{1,2,3} en build/bin
# - targets *-seq usan gcc
# - targets *-par usan mpicc
# - exporta binario con mismo nombre (implX) para calzar con tus scripts
# =========================

# ---------- config de dirs ----------
BIN_DIR      := build/bin
OBJ_DIR      := build/obj
INC_DIR      := include
SRC_DIR      := src

# ---------- fuentes ----------
IMPL1_SEQ_SRC := $(SRC_DIR)/impl1_seq.c
IMPL1_PAR_SRC := $(SRC_DIR)/impl1_par.c
IMPL2_SEQ_SRC := $(SRC_DIR)/impl2_seq.c
IMPL2_PAR_SRC := $(SRC_DIR)/impl2_par.c
IMPL3_SEQ_SRC := $(SRC_DIR)/impl3_seq.c
IMPL3_PAR_SRC := $(SRC_DIR)/impl3_par.c

# objetos (uno por fuente)
IMPL1_SEQ_OBJ := $(OBJ_DIR)/impl1_seq.o
IMPL1_PAR_OBJ := $(OBJ_DIR)/impl1_par.o
IMPL2_SEQ_OBJ := $(OBJ_DIR)/impl2_seq.o
IMPL2_PAR_OBJ := $(OBJ_DIR)/impl2_par.o
IMPL3_SEQ_OBJ := $(OBJ_DIR)/impl3_seq.o
IMPL3_PAR_OBJ := $(OBJ_DIR)/impl3_par.o

# binarios destino (mismo nombre para seq/par según target)
IMPL1_BIN := $(BIN_DIR)/impl1
IMPL2_BIN := $(BIN_DIR)/impl2
IMPL3_BIN := $(BIN_DIR)/impl3

# ---------- toolchains ----------
CC_SEQ   := gcc
CC_PAR   := mpicc

# ---------- flags comunes ----------
CSTD     := -std=c11
WARN     := -Wall -Wextra
OPT      := -O2 -march=native
DEFS     := -D_POSIX_C_SOURCE=200809L
INCS     := -I$(INC_DIR)
DEPFLAGS := -MMD -MP

# activar DES real con: make <target> USE_OPENSSL=1
USE_OPENSSL ?= 0
ifeq ($(USE_OPENSSL),1)
  DEFS     += -DUSE_OPENSSL
  LDL_CRYPTO := -lcrypto
else
  LDL_CRYPTO :=
endif

CFLAGS_SEQ := $(CSTD) $(WARN) $(OPT) $(DEFS) $(INCS) $(DEPFLAGS)
CFLAGS_PAR := $(CFLAGS_SEQ)
CFLAGS_SEQ += -Wno-deprecated-declarations
CFLAGS_PAR += -Wno-deprecated-declarations
LDLIBS_SEQ := $(LDL_CRYPTO)
LDLIBS_PAR := $(LDL_CRYPTO)

# ---------- phony ----------
.PHONY: all all-seq all-par clean rebuild dirs \
        impl1-seq impl1-par impl2-seq impl2-par impl3-seq impl3-par

# build por defecto: sólo secuenciales
all: all-seq

all-seq: dirs impl1-seq impl2-seq impl3-seq
all-par:  dirs impl1-par impl2-par impl3-par

# ---------- reglas de enlace ----------
# nota: cada target (seq/par) sobrescribe el mismo binario final por impl
impl1-seq: $(IMPL1_SEQ_OBJ) | dirs
	$(CC_SEQ) $^ -o $(IMPL1_BIN) $(LDLIBS_SEQ)
	@echo "[ok] $(IMPL1_BIN) (seq)"

impl1-par: $(IMPL1_PAR_OBJ) | dirs
	$(CC_PAR) $^ -o $(IMPL1_BIN) $(LDLIBS_PAR)
	@echo "[ok] $(IMPL1_BIN) (par)"

impl2-seq: $(IMPL2_SEQ_OBJ) | dirs
	$(CC_SEQ) $^ -o $(IMPL2_BIN) $(LDLIBS_SEQ)
	@echo "[ok] $(IMPL2_BIN) (seq)"

impl2-par: $(IMPL2_PAR_OBJ) | dirs
	$(CC_PAR) $^ -o $(IMPL2_BIN) $(LDLIBS_PAR)
	@echo "[ok] $(IMPL2_BIN) (par)"

impl3-seq: $(IMPL3_SEQ_OBJ) | dirs
	$(CC_SEQ) $^ -o $(IMPL3_BIN) $(LDLIBS_SEQ)
	@echo "[ok] $(IMPL3_BIN) (seq)"

impl3-par: $(IMPL3_PAR_OBJ) | dirs
	$(CC_PAR) $^ -o $(IMPL3_BIN) $(LDLIBS_PAR)
	@echo "[ok] $(IMPL3_BIN) (par)"

# ---------- compilación de objetos ----------
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | dirs
	@mkdir -p $(dir $@)
	$(CC_SEQ) $(CFLAGS_SEQ) -c $< -o $@

# si quieres compilar objetos par con mpicc (por si incluyen mpi.h):
$(OBJ_DIR)/impl1_par.o: $(IMPL1_PAR_SRC) | dirs
	$(CC_PAR) $(CFLAGS_PAR) -c $< -o $@

$(OBJ_DIR)/impl2_par.o: $(IMPL2_PAR_SRC) | dirs
	$(CC_PAR) $(CFLAGS_PAR) -c $< -o $@

$(OBJ_DIR)/impl3_par.o: $(IMPL3_PAR_SRC) | dirs
	$(CC_PAR) $(CFLAGS_PAR) -c $< -o $@

# ---------- utilidades ----------
dirs:
	@mkdir -p $(BIN_DIR) $(OBJ_DIR)

clean:
	@$(RM) -r build
	@echo "[clean] build eliminado"

rebuild: clean all

# ---------- ayudas de ejecución rápida (opcionales) ----------
# ejemplo: make impl1-seq USE_OPENSSL=1
# luego: ./scripts/run_seq.sh -i impl1 -h myhost -m a
# para paralelo:
# make impl1-par USE_OPENSSL=1
# ./scripts/run_par.sh -i impl1 -h myhost -m a

# ---------- dependencias automáticas ----------
-include $(IMPL1_SEQ_OBJ:.o=.d)
-include $(IMPL1_PAR_OBJ:.o=.d)
-include $(IMPL2_SEQ_OBJ:.o=.d)
-include $(IMPL2_PAR_OBJ:.o=.d)
-include $(IMPL3_SEQ_OBJ:.o=.d)
-include $(IMPL3_PAR_OBJ:.o=.d)
