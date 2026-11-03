CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -pedantic -O2 -g
TARGET  := leitor-exibidor

# No Windows (MinGW) o gcc gera leitor-exibidor.exe
ifeq ($(OS),Windows_NT)
TARGET  := $(TARGET).exe
endif

SRCS := $(wildcard src/*.c)
OBJS := $(SRCS:src/%.c=build/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all clean test asan cppcheck fuzz

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

build:
	mkdir -p build

# Roda o exibidor em todos os exemplos e resume o resultado.
test: all
	sh scripts/run_examples.sh

# Compila com AddressSanitizer/UBSan para caçar acessos inválidos de memória.
asan: clean
	$(MAKE) CFLAGS="-std=c99 -Wall -Wextra -pedantic -O0 -g -fsanitize=address,undefined"

# ---------------------------------------------------------------------------
# Analisadores combinados com a turma
# ---------------------------------------------------------------------------

# Análise estática. Falha (exit 1) se o cppcheck encontrar qualquer problema.
cppcheck:
	cppcheck --std=c99 --enable=warning,style,performance,portability \
	         --inline-suppr --suppress=missingIncludeSystem \
	         --error-exitcode=1 --quiet -I src src fuzz

# Fuzzing com libFuzzer (precisa do clang). Usa os .class de exemplo como
# sementes e roda por FUZZ_TIME segundos; crashes são salvos em build/fuzz/.
FUZZ_TIME ?= 60
FUZZ_BIN  := build/fuzz/fuzz_classfile
FUZZ_SRCS := fuzz/fuzz_classfile.c $(filter-out src/main.c,$(SRCS))

$(FUZZ_BIN): $(FUZZ_SRCS) $(wildcard src/*.h)
	mkdir -p build/fuzz
	clang -std=c99 -g -O1 -fsanitize=fuzzer,address,undefined \
	      -fno-sanitize-recover=undefined -I src $(FUZZ_SRCS) -o $@

fuzz: $(FUZZ_BIN)
	mkdir -p build/fuzz/corpus
	cp examples/*.class tests/classes/*.class build/fuzz/corpus/
	rm -f build/fuzz/corpus/cafebabe.class
	$(FUZZ_BIN) -max_total_time=$(FUZZ_TIME) -max_len=16384 \
	            -artifact_prefix=build/fuzz/ build/fuzz/corpus

clean:
	rm -rf build $(TARGET) out

-include $(DEPS)
