#include <stdio.h>
#include <stdlib.h>
#include "../include/cpu.h"
#include "../include/loader.h"

int main(int argc, char *argv[]) {
    printf("--- SIMULADOR CPU FASE 1 ---\n");

    // 1. Inicializar Hardware
    inicializar_cpu();

    // 2. Cargar Programa (Hardcodeado por ahora para probar)
    if (!cargar_programa("data/programa1.asm")) {
        printf("[FATAL] Fallo la carga del programa.\n");
        return 1;
    }

    // 3. Mostrar memoria para verificar (Dump)
    dump_cpu();

    // Mostrar las primeras posiciones de memoria de usuario para ver si cargó
    printf("\n--- VISTA DE MEMORIA (Primeras instrucciones) ---\n");
    for(int i = 300; i < 305; i++) {
        printf("Mem[%d] = %08X\n", i, cpu.memoria[i]);
    }

    return 0;
}