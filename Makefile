# =========================
# Proyecto DES bruteforce
# Compila ejecutables impl{1,2,3}_{seq,par} en build/bin
# - *seq usa gcc
# - *par usa mpicc
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

# ---------- objetos ----------
IMPL1_SEQ_OBJ := $(OBJ_DIR)/impl1_seq.o
IMPL1_PAR_OBJ := $(OBJ_DIR)/impl1_par.o
IMPL2_SEQ_OBJ := $(OBJ_DIR)/impl2_seq.o
IMPL2_PAR_OBJ := $(OBJ_DIR)/impl2_par.o
IMPL3_SEQ_OBJ := $(OBJ_DIR)/impl3_seq.o
IMPL3_PAR_OBJ := $(OBJ_DIR)/impl3_par.o

# ---------- binarios destino (separados) ----------
IMPL1_BIN_SEQ := $(BIN_DIR)/impl1_seq
IMPL1_BIN_PAR := $(BIN_DIR)/impl1_par
IMPL2_BIN_SEQ := $(BIN_DIR)/impl2_seq
IMPL2_BIN_PAR := $(BIN_DIR)/impl2_par
IMPL3_BIN_SEQ := $(BIN_DIR)/impl3_seq
IMPL3_BIN_PAR := $(BIN_DIR)/impl3_par

# ---------- toolchains ----------
CC_SEQ := gcc
CC_PAR := mpicc

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
  DEFS       += -DUSE_OPENSSL
  LDL_CRYPTO := -lcrypto
else
  LDL_CRYPTO :=
endif

CFLAGS_BASE := $(CSTD) $(WARN) $(OPT) $(DEFS) $(INCS) $(DEPFLAGS) -Wno-deprecated-declarations
CFLAGS_SEQ  := $(CFLAGS_BASE)
CFLAGS_PAR  := $(CFLAGS_BASE)
LDLIBS_SEQ  := $(LDL_CRYPTO)
LDLIBS_PAR  := $(LDL_CRYPTO)

# ---------- phony ----------
.PHONY: all all-seq all-par clean rebuild dirs \
        impl1-seq impl1-par impl2-seq impl2-par impl3-seq impl3-par

# build por defecto: todo
all: all-seq all-par
all-seq: dirs impl1-seq impl2-seq impl3-seq
all-par:  dirs impl1-par impl2-par impl3-par

# ---------- reglas de enlace ----------
impl1-seq: $(IMPL1_SEQ_OBJ) | dirs
	$(CC_SEQ) $^ -o $(IMPL1_BIN_SEQ) $(LDLIBS_SEQ)
	@echo "[ok] $(IMPL1_BIN_SEQ)"

impl1-par: $(IMPL1_PAR_OBJ) | dirs
	$(CC_PAR) $^ -o $(IMPL1_BIN_PAR) $(LDLIBS_PAR)
	@echo "[ok] $(IMPL1_BIN_PAR)"

impl2-seq: $(IMPL2_SEQ_OBJ) | dirs
	$(CC_SEQ) $^ -o $(IMPL2_BIN_SEQ) $(LDLIBS_SEQ)
	@echo "[ok] $(IMPL2_BIN_SEQ)"

impl2-par: $(IMPL2_PAR_OBJ) | dirs
	$(CC_PAR) $^ -o $(IMPL2_BIN_PAR) $(LDLIBS_PAR)
	@echo "[ok] $(IMPL2_BIN_PAR)"

impl3-seq: $(IMPL3_SEQ_OBJ) | dirs
	$(CC_SEQ) $^ -o $(IMPL3_BIN_SEQ) $(LDLIBS_SEQ)
	@echo "[ok] $(IMPL3_BIN_SEQ)"

impl3-par: $(IMPL3_PAR_OBJ) | dirs
	$(CC_PAR) $^ -o $(IMPL3_BIN_PAR) $(LDLIBS_PAR)
	@echo "[ok] $(IMPL3_BIN_PAR)"

# ---------- compilación de objetos ----------
# regla general (secuencial)
$(OBJ_DIR)/%_seq.o: $(SRC_DIR)/%_seq.c | dirs
	@mkdir -p $(dir $@)
	$(CC_SEQ) $(CFLAGS_SEQ) -c $< -o $@

# regla general (paralelo, incluye mpi.h)
$(OBJ_DIR)/%_par.o: $(SRC_DIR)/%_par.c | dirs
	@mkdir -p $(dir $@)
	$(CC_PAR) $(CFLAGS_PAR) -c $< -o $@

# ---------- utilidades ----------
dirs:
	@mkdir -p $(BIN_DIR) $(OBJ_DIR)

clean:
	@$(RM) -r build
	@echo "[clean] build eliminado"

rebuild: clean all

# ---------- dependencias automáticas ----------
-include $(IMPL1_SEQ_OBJ:.o=.d) $(IMPL1_PAR_OBJ:.o=.d) \
          $(IMPL2_SEQ_OBJ:.o=.d) $(IMPL2_PAR_OBJ:.o=.d) \
          $(IMPL3_SEQ_OBJ:.o=.d) $(IMPL3_PAR_OBJ:.o=.d)
