#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/cpu.h"
#include "../include/constantes.h"
#include "../include/loader.h"
#include "../include/logger.h"

int cargar_programa(const char *nombre_archivo) {
    FILE *archivo;
    char linea[256];
    int direccion_actual = INICIO_USUARIO; // Empezamos a cargar desde la 300
    int instruccion;

    logger_log("[LOADER] Abriendo archivo: %s\n", nombre_archivo);

    archivo = fopen(nombre_archivo, "r");
    if (archivo == NULL) {
        perror("[ERROR] No se pudo abrir el archivo");
        return 0; // Fallo
    }

    // Leemos línea por línea
    while (fgets(linea, sizeof(linea), archivo)) {
        // Ignoramos líneas que empiezan con punto (.) o guion bajo (_)
        // o que están vacías
        if (linea[0] == '.' || linea[0] == '_' || linea[0] == '\n' || linea[0] == ' ') {
            continue;
        }

        // Intentamos leer un número decimal de 8 dígitos al inicio de la línea
        // %d lee decimales
        if (sscanf(linea, "%d", &instruccion) == 1) {
            // Verificar que no desbordemos la memoria
            if (direccion_actual >= TAMANO_MEMORIA) {
                logger_log("[ERROR] El programa es demasiado grande para la memoria.\n");
                break;
            }

            // Guardamos en la RAM de nuestra CPU
            cpu.memoria[direccion_actual] = instruccion;
            
            logger_log("[MEM] Dir %04d: %08d cargado.\n", direccion_actual, instruccion);
            
            direccion_actual++;
        }
    }

    fclose(archivo);
    logger_log("[LOADER] Carga completada. %d instrucciones cargadas.\n", direccion_actual - INICIO_USUARIO);

    return 1; // Éxito
}