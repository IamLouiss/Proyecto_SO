#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/cpu.h"
#include "../include/loader.h"
#include "../include/logger.h"

// Funcion para cargar el programa:
int cargar_programa(const char *nombre_archivo, int direccion_carga, int *pc_inicial_out, char *nombre_prog_out, int *num_palabras_out) {
    FILE *archivo = fopen(nombre_archivo, "r"); // Abrimos el archivo para lectura
    if (!archivo) {
        logger_log("Error: No se pudo abrir %s\n", nombre_archivo);
        printf("Error: No se pudo abrir: %s\n", nombre_archivo);
        return -1;
    }

    logger_log("Loader: Cargando '%s' en RAM[%d]\n", nombre_archivo, direccion_carga);

    char linea[256];
    int dirrecion_actual = direccion_carga;
    int instrucciones_cargadas = 0;
    
    // Valores por defecto
    *pc_inicial_out = 0; 
    strcpy(nombre_prog_out, "SinNombre");
    *num_palabras_out = 0;

    // 1. Lectura de encabezado con datos del programa
    // lineaa 1: _start
    if (fgets(linea, sizeof(linea), archivo)) {
        sscanf(linea, "_start %d", pc_inicial_out);
    }
    
    // lineaa 2: .NumeroPalabras
    if (fgets(linea, sizeof(linea), archivo)) {
        sscanf(linea, ".NumeroPalabras %d", num_palabras_out);
    }

    // lineaa 3: .NombreProg
    if (fgets(linea, sizeof(linea), archivo)) {
        sscanf(linea, ".NombreProg %s", nombre_prog_out);
    }

    // 2. Lectura de instrucciones
    while (fgets(linea, sizeof(linea), archivo)) {
        
        // Cortamos comentarios al final de la lineaa (si hay "0410005 // comentario")
        char *token = strtok(linea, " \t\r\n/"); 
        
        if (token) {
            // Verificamos que sea un digito (instruccion)
            if (token[0] >= '0' && token[0] <= '9') {
                
                if (dirrecion_actual < TAMANO_MEMORIA) {
                    // Convertimos a entero la instruccion
                    int instruccion = atoi(token);
                    
                    cpu.memoria[dirrecion_actual] = instruccion; // Guardamos la instruccion en la memoria
                    
                    // Registramos en el log la carga de cada instruccion
                    logger_log("[MEM] RAM[%04d] = %08d\n", dirrecion_actual, instruccion);
                    
                    dirrecion_actual++;
                    instrucciones_cargadas++;
                } else {
                    printf("Error: Memoria llena (Overflow).\n");
                    fclose(archivo);
                    return -1;
                }
            }
        }
    }

    fclose(archivo);
    return instrucciones_cargadas;
}

// Funcion auxiliar para tomar datos del archivo antes de cargarlo
int obtener_datos_programa(const char *nombre_archivo, char *nombre_out, int *tamano_out) {
    FILE *archivo = fopen(nombre_archivo, "r");
    if (!archivo) {
        printf("Error: No se pudo abrir el archivo: %s\n", nombre_archivo);
        return -1; // Si no se pudo abrir el archivo retorna -1
    }
    
    // Valores por defecto
    char linea[256];
    *tamano_out = 0; // Puntero para poder "retornar" el valor
    strcpy(nombre_out, "Desconocido");

    // Leemos lineaa 1: _start para avanzar, pero no guardamos el dato aqui
    if (fgets(linea, sizeof(linea), archivo) == NULL) {
        printf("Error al leer el archivo");
        fclose(archivo);
        return 0;
    }

    // Leemos lineaa 2: .NumeroPalabras
    if (fgets(linea, sizeof(linea), archivo) != NULL) {
        sscanf(linea, ".NumeroPalabras %d", tamano_out); // Extraemos solo el numero
    }

    // Leemos lineaa 3: .NombreProg
    if (fgets(linea, sizeof(linea), archivo) != NULL) {
        sscanf(linea, ".NombreProg %s", nombre_out); // Extraemos solo el nombre
    }

    fclose(archivo);

    // Retornamos el tamaño si es valido (>0), si no, error (0)
    return (*tamano_out > 0) ? *tamano_out : 0;
}