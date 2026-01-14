#include <stdio.h>
#include <stdarg.h>
#include "../include/logger.h"

// Puntero al archivo de log (privado para este modulo)
static FILE *log_file = NULL;

void logger_init(const char *filename) {
    log_file = fopen(filename, "w"); 
    if (log_file == NULL) {
        perror("Error al crear el archivo de log");
    }
}

void logger_log(const char *format, ...) {
    va_list args;

    // 1. Escribir en la CONSOLA (Pantalla)
    // USAMOS vprintf (Estándar de C) -> NO CAMBIAR
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    // 2. Escribir en el ARCHIVO 
    if (log_file != NULL) {
        // USAMOS vfprintf (Estándar de C) -> NO CAMBIAR
        va_start(args, format);
        vfprintf(log_file, format, args);
        va_end(args);
        
        // Forzamos guardado
        fflush(log_file);
    }
}

void logger_close() {
    if (log_file != NULL) {
        fclose(log_file);
        log_file = NULL;
    }
}