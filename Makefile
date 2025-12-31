# Nombre del ejecutable
TARGET = bin/simulador

# Compilador y opciones
CC = gcc
CFLAGS = -Wall -Iinclude -pthread

# Archivos fuente
SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:src/%.c=obj/%.o)

# Regla principal (lo que pasa al escribir 'make')
all: directories $(TARGET)

# Linkeo final
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compilación de cada archivo .c a .o
obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Crear carpetas si no existen
directories:
	mkdir -p bin obj logs

# Limpiar (make clean)
clean:
	rm -rf bin/* obj/* logs/*

.PHONY: all clean directories