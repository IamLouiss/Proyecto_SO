#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/cpu.h"
#include "../include/loader.h"
#include "../include/logger.h"

int cargar_programa(const char *filename, int direccion_carga, int *pc_inicial_out, char *nombre_prog_out, int *num_palabras_out) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("[ERROR] No se pudo abrir el archivo: %s\n", filename);
        return -1;
    }

    logger_log("[LOADER] Iniciando carga de '%s' en Memoria[%d]\n", filename, direccion_carga);

    char line[256];
    int current_addr = direccion_carga;
    
    // Valores por defecto si no vienen en el archivo
    *pc_inicial_out = 0; 
    strcpy(nombre_prog_out, "Desconocido");
    *num_palabras_out = 0; 

    // --- PRIMERA PASADA: LEER METADATOS Y CARGAR CÓDIGO ---
    while (fgets(line, sizeof(line), file)) {
        
        // Limpieza de saltos de línea
        line[strcspn(line, "\r\n")] = 0;

        // Ignorar líneas vacías o comentarios puros
        if (line[0] == '\0' || line[0] == '/') continue;

        // 1. Detectar _start
        if (strncmp(line, "_start", 6) == 0) {
            sscanf(line, "_start %d", pc_inicial_out);
            continue;
        }

        // 2. Detectar .NombreProg
        if (strncmp(line, ".NombreProg", 11) == 0) {
            char buffer_nombre[50];
            if (sscanf(line, ".NombreProg %s", buffer_nombre) == 1) {
                strcpy(nombre_prog_out, buffer_nombre);
            }
            continue;
        }

        // 3. Detectar .NumeroPalabras
        if (strncmp(line, ".NumeroPalabras", 15) == 0) {
            int val;
            if (sscanf(line, ".NumeroPalabras %d", &val) == 1) {
                *num_palabras_out = val;
            }
            continue;
        }

        // 4. Cargar Instrucciones (Números)
        // Si la línea empieza con un dígito, es instrucción
        if (line[0] >= '0' && line[0] <= '9') {
            if (current_addr < TAMANO_MEMORIA) {
                // Solo tomamos la primera palabra de la línea
                char *token = strtok(line, " \t");
                if (token) {
                    cpu.memoria[current_addr] = atoi(token);
                    logger_log("[MEM] Dir %04d: %08d cargado\n", current_addr, cpu.memoria[current_addr]);
                    current_addr++;
                }
            } else {
                printf("[ERROR] Memoria llena.\n");
                fclose(file);
                return -1;
            }
        }
    }

    fclose(file);
    
    // Validación de seguridad:
    // Si el archivo no traía .NumeroPalabras, usamos lo que contamos al cargar
    int instrucciones_reales = current_addr - direccion_carga;
    if (*num_palabras_out == 0) {
        *num_palabras_out = instrucciones_reales;
    }

    return 0; // Éxito
}