#include <stdio.h>
#include <stdlib.h> // Para atoi y rand
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "disco.h"
#include "cpu.h"    
#include "logger.h" 

// Definición de las variables globales
Disco_t disco;
DMA_t dma;

void inicializar_disco() {
    // Llenamos el disco de ceros para limpiar basura
    memset(&disco, 0, sizeof(Disco_t));
    
    // Inicializamos el DMA
    dma.pista_seleccionada = 0;
    dma.cilindro_seleccionado = 0;
    dma.sector_seleccionado = 0;
    dma.direccion_memoria = 0;
    dma.es_escritura = 0;
    dma.estado = 0; // 0 = Éxito por defecto
    dma.activo = 0;
    
    logger_log("[DISCO] Hardware inicializado (10 pistas, 10 cilindros, 100 sectores).\n");
}

// --- HILO DEL DMA ---
void *hilo_dma(void *arg) {
    CPU_t *cpu_ptr = (CPU_t *)arg;

    while (cpu_ptr->ejecutando) {
        
        // 1. Polling: Esperamos a que la instrucción SDMAON active la bandera
        if (dma.activo) {
            
            // Requisito PDF: Comunicación con el disco se deja al diseñador.
            // Simulamos latencia mecánica (50ms).
            usleep(50000); 

            // 2. Sección Crítica: Acceso a Memoria (Arbitraje del Bus)
            // Requisito PDF: "Debe haber algún tipo de arbitraje"
            pthread_mutex_lock(&cpu_ptr->mutex);

            // Verificación de coordenadas (Simulación de hardware)
            if (dma.pista_seleccionada >= DISCO_PISTAS || 
                dma.cilindro_seleccionado >= DISCO_CILINDROS || 
                dma.sector_seleccionado >= DISCO_SECTORES) {
                    
                // Requisito PDF: "ESTADOdma... 1=error"
                dma.estado = 1; 
                logger_log("[DMA] Error: Coordenadas invalidas (%d, %d, %d)\n", 
                    dma.pista_seleccionada, dma.cilindro_seleccionado, dma.sector_seleccionado);
            } else {
                // Requisito PDF: "ESTADOdma... 0=éxito"
                dma.estado = 0;

                // Puntero al sector físico
                Sector_t *sector = &disco.plato[dma.pista_seleccionada][dma.cilindro_seleccionado][dma.sector_seleccionado];
                int dir = dma.direccion_memoria;

                // Requisito PDF: Validar direccionamiento de memoria (Protección)
                // Aunque el DMA suele saltarse esto, para el simulador es bueno validar que 'dir' existe.
                if (dir < 0 || dir >= TAMANO_MEMORIA) {
                     dma.estado = 1;
                     logger_log("[DMA] Error: Direccion de RAM invalida (%d)\n", dir);
                } else {
                    if (dma.es_escritura == 1) { // 1 = Escribir (RAM -> DISCO)
                        snprintf(sector->datos, TAMANO_SECTOR, "%d", cpu_ptr->memoria[dir]);
                        logger_log("[DMA] WRITE: RAM[%d] (%d) -> Disco[%d][%d][%d]\n",
                            dir, cpu_ptr->memoria[dir], dma.pista_seleccionada, dma.cilindro_seleccionado, dma.sector_seleccionado);
                    } else { // 0 = Leer (DISCO -> RAM)
                        // Convertimos el string del sector a entero
                        int valor = atoi(sector->datos);
                        cpu_ptr->memoria[dir] = valor;
                        logger_log("[DMA] READ: Disco[%d][%d][%d] ('%s') -> RAM[%d] (%d)\n",
                            dma.pista_seleccionada, dma.cilindro_seleccionado, dma.sector_seleccionado, sector->datos, dir, valor);
                    }
                }
            }

            // 3. Requisito PDF: "Luego, interrumpe al procesador" (Código 4)
            // Usamos la bandera que ya tienes implementada en cpu.c
            if (cpu_ptr->interrupcion_pendiente == 0) {
                cpu_ptr->interrupcion_pendiente = 1;
                cpu_ptr->codigo_interrupcion = 4; // 4 = Fin de E/S
            }

            dma.activo = 0; // Apagamos el DMA y liberamos el bus
            pthread_mutex_unlock(&cpu_ptr->mutex);
        
        } else {
            // Ahorro de CPU mientras espera
            usleep(10000); 
        }
    }
    return NULL;
}