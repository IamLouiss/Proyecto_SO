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

    // --- MODIFICACIÓN: YA NO IMPRIMIMOS EN CONSOLA ---
    // El PDF pide que el log registre todo, pero la consola solo muestre
    // lo específico (Interrupciones y Debugger).
    // Por eso, eliminamos el vprintf de aquí.
    
    // 2. Escribir SOLO en el ARCHIVO 
    if (log_file != NULL) {
        va_start(args, format);
        vfprintf(log_file, format, args);
        va_end(args);
        
        // Forzamos guardado para que el log se actualice en tiempo real
        fflush(log_file);
    }
}

void logger_close() {
    if (log_file != NULL) {
        fclose(log_file);
        log_file = NULL;
    }
}