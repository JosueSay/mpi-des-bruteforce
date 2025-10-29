# ---------- dirs ----------
BIN_DIR      := build/bin
OBJ_DIR      := build/obj
INC_DIR      := include
SRC_DIR      := src

# ---------- fuentes ----------
CORE_UTILS_SRC  := $(SRC_DIR)/core_utils.c
CORE_CRYPTO_SRC := $(SRC_DIR)/core_crypto.c

IMPL1_SEQ_SRC := $(SRC_DIR)/impl1_seq.c
IMPL1_PAR_SRC := $(SRC_DIR)/impl1_par.c
IMPL2_SEQ_SRC := $(SRC_DIR)/impl2_seq.c
IMPL2_PAR_SRC := $(SRC_DIR)/impl2_par.c
IMPL3_SEQ_SRC := $(SRC_DIR)/impl3_seq.c
IMPL3_PAR_SRC := $(SRC_DIR)/impl3_par.c

# ---------- objetos ----------
CORE_UTILS_OBJ  := $(OBJ_DIR)/core_utils.o
CORE_CRYPTO_OBJ := $(OBJ_DIR)/core_crypto.o
CORE_OBJS       := $(CORE_UTILS_OBJ) $(CORE_CRYPTO_OBJ)

IMPL1_SEQ_OBJ := $(OBJ_DIR)/impl1_seq.o
IMPL1_PAR_OBJ := $(OBJ_DIR)/impl1_par.o
IMPL2_SEQ_OBJ := $(OBJ_DIR)/impl2_seq.o
IMPL2_PAR_OBJ := $(OBJ_DIR)/impl2_par.o
IMPL3_SEQ_OBJ := $(OBJ_DIR)/impl3_seq.o
IMPL3_PAR_OBJ := $(OBJ_DIR)/impl3_par.o

# ---------- binarios ----------
IMPL1_SEQ_BIN := $(BIN_DIR)/impl1_seq
IMPL1_PAR_BIN := $(BIN_DIR)/impl1_par
IMPL2_SEQ_BIN := $(BIN_DIR)/impl2_seq
IMPL2_PAR_BIN := $(BIN_DIR)/impl2_par
IMPL3_SEQ_BIN := $(BIN_DIR)/impl3_seq
IMPL3_PAR_BIN := $(BIN_DIR)/impl3_par

# ---------- toolchains ----------
CC_SEQ := gcc
CC_PAR := mpicc

# ---------- flags ----------
CSTD     := -std=c11
WARN     := -Wall -Wextra
OPT      := -O2 -march=native
DEFS     := -D_POSIX_C_SOURCE=200809L -DUSE_OPENSSL
INCS     := -I$(INC_DIR)
DEPFLAGS := -MMD -MP

# OpenSSL forzado
LDL_CRYPTO := -lcrypto

CFLAGS_SEQ := $(CSTD) $(WARN) $(OPT) $(DEFS) $(INCS) $(DEPFLAGS) -Wno-deprecated-declarations
CFLAGS_PAR := $(CFLAGS_SEQ)
LDLIBS_SEQ := $(LDL_CRYPTO)
LDLIBS_PAR := $(LDL_CRYPTO)

# ---------- phony ----------
.PHONY: all all-seq all-par clean rebuild dirs \
        impl1-seq impl1-par impl2-seq impl2-par impl3-seq impl3-par

# build por defecto: secuenciales
all: all-seq
all-seq: dirs impl1-seq impl2-seq impl3-seq
all-par:  dirs impl1-par impl2-par impl3-par

# ---------- enlace ----------
impl1-seq: $(CORE_OBJS) $(IMPL1_SEQ_OBJ) | dirs
	$(CC_SEQ) $^ -o $(IMPL1_SEQ_BIN) $(LDLIBS_SEQ)
	@echo "[ok] $(IMPL1_SEQ_BIN)"

impl1-par: $(CORE_OBJS) $(IMPL1_PAR_OBJ) | dirs
	$(CC_PAR) $^ -o $(IMPL1_PAR_BIN) $(LDLIBS_PAR)
	@echo "[ok] $(IMPL1_PAR_BIN)"

impl2-seq: $(CORE_OBJS) $(IMPL2_SEQ_OBJ) | dirs
	$(CC_SEQ) $^ -o $(IMPL2_SEQ_BIN) $(LDLIBS_SEQ)
	@echo "[ok] $(IMPL2_SEQ_BIN)"

impl2-par: $(CORE_OBJS) $(IMPL2_PAR_OBJ) | dirs
	$(CC_PAR) $^ -o $(IMPL2_PAR_BIN) $(LDLIBS_PAR)
	@echo "[ok] $(IMPL2_PAR_BIN)"

impl3-seq: $(CORE_OBJS) $(IMPL3_SEQ_OBJ) | dirs
	$(CC_SEQ) $^ -o $(IMPL3_SEQ_BIN) $(LDLIBS_SEQ)
	@echo "[ok] $(IMPL3_SEQ_BIN)"

impl3-par: $(CORE_OBJS) $(IMPL3_PAR_OBJ) | dirs
	$(CC_PAR) $^ -o $(IMPL3_PAR_BIN) $(LDLIBS_PAR)
	@echo "[ok] $(IMPL3_PAR_BIN)"

# ---------- compilación objetos ----------
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | dirs
	@mkdir -p $(dir $@)
	$(CC_SEQ) $(CFLAGS_SEQ) -c $< -o $@

# par con mpicc (solo los que incluyen mpi.h)
$(OBJ_DIR)/impl1_par.o: $(IMPL1_PAR_SRC) | dirs
	$(CC_PAR) $(CFLAGS_PAR) -c $< -o $@

$(OBJ_DIR)/impl2_par.o: $(IMPL2_PAR_SRC) | dirs
	$(CC_PAR) $(CFLAGS_PAR) -c $< -o $@

$(OBJ_DIR)/impl3_par.o: $(IMPL3_PAR_SRC) | dirs
	$(CC_PAR) $(CFLAGS_PAR) -c $< -o $@

# ---------- util ----------
dirs:
	@mkdir -p $(BIN_DIR) $(OBJ_DIR)

clean:
	@$(RM) -r build
	@echo "[clean] build eliminado"

rebuild: clean all

# ---------- deps ----------
-include $(CORE_UTILS_OBJ:.o=.d)
-include $(CORE_CRYPTO_OBJ:.o=.d)
-include $(IMPL1_SEQ_OBJ:.o=.d)
-include $(IMPL1_PAR_OBJ:.o=.d)
-include $(IMPL2_SEQ_OBJ:.o=.d)
-include $(IMPL2_PAR_OBJ:.o=.d)
-include $(IMPL3_SEQ_OBJ:.o=.d)
-include $(IMPL3_PAR_OBJ:.o=.d)
