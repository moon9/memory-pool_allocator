.PHONY: build_static_lib build_test

all: build_static_lib build_test

TARGET_DIR ?= .
CROSS_COMPILE ?=
CC ?= gcc
AR ?= ar
DEBUG ?= n
THREAD_SAFE ?= n

BUILD_DIR := $(TARGET_DIR)/build
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := $(BUILD_DIR)/bin
SRC_DIR := ./
TEST_SRC_DIR := ./test

AR_NAME := libpa.a

TEST_BIN := libpa_test

CFLAGS := -c -O3 -I./ -std=c99 -Wall -Werror -D_POSIX_C_SOURCE=199309L

ifeq ($(DEBUG),y)
    CFLAGS += -O0 -g -D_DEBUG
endif

SRC := $(SRC_DIR)/pool_allocator.c

OBJ := $(subst $(SRC_DIR),$(OBJ_DIR),$(SRC))
OBJ := $(patsubst %.c,%.o,$(OBJ))

TEST_SRC := $(TEST_SRC_DIR)/main.c

TEST_OBJ := $(subst $(TEST_SRC_DIR),$(OBJ_DIR)/test,$(TEST_SRC))
TEST_OBJ := $(patsubst %.c,%.o,$(TEST_OBJ))

TEST_LDFLAGS := -L$(BIN_DIR) -lpa

$(BIN_DIR)/$(AR_NAME): $(OBJ) $(SRC)
	@mkdir -p $(@D)
	@echo "[AR] $@"
	@$(CROSS_COMPILE)$(AR) rcs $@ $(OBJ)

$(BIN_DIR)/$(TEST_BIN): $(TEST_OBJ) $(TEST_SRC)
	@mkdir -p $(@D)
	@echo "[LD] $@"
	@$(CROSS_COMPILE)$(CC) -o $@ $(TEST_OBJ) $(TEST_LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	@echo "[CC] $@"
	@$(CROSS_COMPILE)$(CC) $(CFLAGS) $< -o $@

$(OBJ_DIR)/test/%.o: $(TEST_SRC_DIR)/%.c
	@mkdir -p $(@D)
	@echo "[CC] $@"
	@$(CROSS_COMPILE)$(CC) $(CFLAGS) $< -o $@

build_static_lib: $(BIN_DIR)/$(AR_NAME)

build_test: $(BIN_DIR)/$(TEST_BIN)
