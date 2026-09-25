.DEFAULT_GOAL := test
.DELETE_ON_ERROR:

CC = cc
CFLAGS ?= -O0 -g -Wall -Wextra
PYTHON ?= python3
DOCKER ?= docker
STELLA_IMAGE := fizruk/stella@sha256:86d6123aff226787a02b1b8021c7d4f2cd3703586155f5a96d6f0a7c21d805c6
COMPILE_FLAGS = $(CPPFLAGS) -Iinclude $(CFLAGS) -std=c11

C_TESTS := $(patsubst tests/c/%.c,build/tests/c/%,$(wildcard tests/c/*.c))
STELLA_TESTS := $(patsubst tests/stella/%.json,build/tests/stella/%,$(wildcard tests/stella/*.json))
COMMON_OBJECTS := build/obj/src/gc.o build/obj/runtime/runtime.o build/obj/runtime/scalar.o
MAIN_OBJECT := build/obj/runtime/main.o
OBJECTS := $(COMMON_OBJECTS) $(MAIN_OBJECT) $(patsubst tests/c/%.c,build/obj/tests/c/%.o,$(wildcard tests/c/*.c))

.PHONY: all test test-c test-stella clean compdb
# Editor tooling is local and may be absent in a fresh checkout.
ifneq ($(wildcard .tools/compile_commands.py),)
all test-c test-stella: compdb
endif

# Refresh clangd's database, including when sources or compiler flags change.
compdb: export COMPDB_CC = $(CC)
compdb: export COMPDB_FLAGS = $(COMPILE_FLAGS)
compdb:
	$(PYTHON) .tools/compile_commands.py

all: $(C_TESTS) $(STELLA_TESTS)

test: test-c test-stella

test-c: $(C_TESTS)
	$(PYTHON) tests/run.py c $(C_TESTS)

test-stella: $(STELLA_TESTS)
	$(PYTHON) tests/run.py stella $(wildcard tests/stella/*.json)

build/obj/%.o: %.c Makefile
	@mkdir -p $(@D)
	$(CC) $(COMPILE_FLAGS) -MMD -MP -c $< -o $@

build/tests/c/%: build/obj/tests/c/%.o $(COMMON_OBJECTS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

# Write atomically: a failed compiler must never leave a cached, partial C file.
build/generated/%.c: tests/stella/%.stella Makefile
	@mkdir -p $(@D)
	$(DOCKER) run --rm -i $(STELLA_IMAGE) compile /dev/stdin -o /dev/stdout < $< > $@.tmp || { rm -f $@.tmp; exit 1; }
	mv $@.tmp $@

build/tests/stella/%: build/generated/%.c $(COMMON_OBJECTS) $(MAIN_OBJECT)
	@mkdir -p $(@D)
	$(CC) $(COMPILE_FLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

# Keep generated C and test objects for incremental builds and inspection.
.SECONDARY:
-include $(OBJECTS:.o=.d)

clean:
	rm -rf build
