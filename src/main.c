#include <stdio.h>
#include <stdlib.h>
#include "../include/cpu.h"
#include "../include/loader.h"

int main(int argc, char *argv[]) {
    printf("--- SIMULADOR CPU FASE 1 ---\n");

    // 1. Inicializar Hardware
    inicializar_cpu();

    // 2. Cargar Programa
    if (!cargar_programa("data/programa1.asm")) {
        printf("[FATAL] Fallo la carga del programa.\n");
        return 1;
    }

    // Opcional: Mostrar estado del cpu antes de arrancar
    dump_cpu();

    // Arrancar el cpu
    ejecutar_cpu();

    // Opcional: Mostrar estado final del cpu
    dump_cpu();

    // Mostrar las primeras posiciones de memoria de usuario para ver si cargó
    // printf("\n--- VISTA DE MEMORIA (Primeras instrucciones) ---\n");
    // for(int i = 300; i < 305; i++) {
    //     printf("Mem[%d] = %08X\n", i, cpu.memoria[i]);
    // }

    return 0;
}