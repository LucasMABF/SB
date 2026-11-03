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

.PHONY: all clean test asan

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

clean:
	rm -rf build $(TARGET) out

-include $(DEPS)
