#include <stdio.h>
#include <stdarg.h>
#include "../include/logger.h"

// Puntero al archivo de log (privado para este modulo)
static FILE *log_archivo = NULL;

void logger_init(const char *nombre_archivo) {
    log_archivo = fopen(nombre_archivo, "w"); 
    if (log_archivo == NULL) {
        perror("Error al crear el archivo de log");
    }
}

void logger_log(const char *format, ...) {
    // Esta variable sirve para meter los argumentos del parametro "..."
    // De manera que podamos escribir en el archivo como lo hace la funcion printf,
    // la cual puede tomar cualquier cantidad de argumentos
    va_list args;
    
    // Escribimos en el archivo .log 
    if (log_archivo != NULL) {
        va_start(args, format); // Con esto "abrimos" la bolsa de argumentos
        // Usamos vfprintf para escribir en el formato indicado con los argumentos variables
        vfprintf(log_archivo, format, args);
        va_end(args); // "Cerramos" la bolsa de argumentos y limpiamos la memoria   
        
        // Forzamos guardado para que el log se actualice en tiempo real
        fflush(log_archivo);
    }
}

void logger_close() {
    if (log_archivo != NULL) {
        fclose(log_archivo);
        log_archivo = NULL;
    }
}