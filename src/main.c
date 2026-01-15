#include <stdio.h>
#include <stdlib.h>
#include "../include/cpu.h"
#include "../include/loader.h"
#include "../include/logger.h"

int main(int argc, char *argv[]) {
    logger_init("logs/simulador.log");
    logger_log("--- INICIO DEL SIMULADOR ---\n");

    // 1. Inicializar Hardware
    inicializar_cpu();
    
    // Inicializamos Mutex
    pthread_mutex_init(&cpu.mutex, NULL);
    cpu.timer_periodo = 0; // Timer apagado por defecto
    cpu.interrupcion_pendiente = 0;

    // 2. Cargar Programa
    if (!cargar_programa("data/programa1.asm")) {
        logger_log("[FATAL] Fallo la carga del programa.\n");
        return 1;
    }

    // CREAR EL HILO DEL TIMER (Nace el paralelismo)
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, hilo_timer, &cpu) != 0) {
        logger_log("[ERROR] No se pudo crear el hilo del Timer.\n");
        return 1;
    }
    logger_log("[INFO] Hilo del Timer iniciado correctamente.\n");

    // Opcional: Mostrar estado del cpu antes de arrancar
    dump_cpu();

    // Arrancar el cpu
    ejecutar_cpu();

    // Esperamos al hilo y limpiamos
    pthread_join(thread_id, NULL);
    pthread_mutex_destroy(&cpu.mutex);

    // Opcional: Mostrar estado final del cpu
    dump_cpu();

    // Mostrar las primeras posiciones de memoria de usuario para ver si cargó
    // logger_log("\n--- VISTA DE MEMORIA (Primeras instrucciones) ---\n");
    // for(int i = 300; i < 305; i++) {
    //     logger_log("Mem[%d] = %08X\n", i, cpu.memoria[i]);
    // }

    logger_log("--- FIN DE LA EJECUCION ---\n");
    logger_close();

    return 0;
}