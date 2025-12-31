#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/cpu.h"
#include "../include/constantes.h"
#include "../include/loader.h"

int cargar_programa(const char *nombre_archivo) {
    FILE *archivo;
    char linea[256];
    int direccion_actual = INICIO_USUARIO; // Empezamos a cargar desde la 300
    int instruccion;

    printf("[LOADER] Abriendo archivo: %s\n", nombre_archivo);

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

        // Intentamos leer un número hexadecimal de 8 dígitos al inicio de la línea
        // %x lee hexadecimales
        if (sscanf(linea, "%x", &instruccion) == 1) {
            // Verificar que no desbordemos la memoria
            if (direccion_actual >= TAMANO_MEMORIA) {
                printf("[ERROR] El programa es demasiado grande para la memoria.\n");
                break;
            }

            // Guardamos en la RAM de nuestra CPU
            cpu.memoria[direccion_actual] = instruccion;
            
            printf("[MEM] Dir %04d: %08X cargado.\n", direccion_actual, instruccion);
            
            direccion_actual++;
        }
    }

    fclose(archivo);
    printf("[LOADER] Carga completada. %d instrucciones cargadas.\n", direccion_actual - INICIO_USUARIO);
    
    // Actualizamos el RL (Registro Límite) al tamaño real del programa cargado
    cpu.RL = direccion_actual - 1;
    
    return 1; // Éxito
}