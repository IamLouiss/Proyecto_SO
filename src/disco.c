#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "../include/disco.h"
#include "../include/cpu.h"    
#include "../include/logger.h" 

// Definicion de las variables globales
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
    dma.estado = 0;
    dma.activo = 0;
    
    logger_log("Disco: Hardware inicializado (10 pistas, 10 cilindros, 100 sectores).\n");
}

// Hilo del DMA
void *hilo_dma(void *arg) {
    // Como los hilos trabajan con punteros genericos de tipo void, casteamos
    // para que lo interprete como puntero a una estructura de tipo CPU
    CPU_t *cpu_ptr = (CPU_t *)arg;

    while (1) { 
        
        // Bloqueamos el mutex antes
        pthread_mutex_lock(&cpu_ptr->mutex);

        // Dormimos el hilo para que espere sin consumir CPU hasta que la instruccion
        // SDMAON active la bandera
        while (dma.activo == 0 && cpu_ptr->ejecutando) {
            pthread_cond_wait(&cpu_ptr->cond_dma, &cpu_ptr->mutex);
        }

        // Verificamos si despertamos porque se acabo el programa
        if (!cpu_ptr->ejecutando) {
            pthread_mutex_unlock(&cpu_ptr->mutex);
            break;
        }

        // Verificacion de coordenadas
        if (dma.pista_seleccionada >= DISCO_PISTAS || 
            dma.cilindro_seleccionado >= DISCO_CILINDROS || 
            dma.sector_seleccionado >= DISCO_SECTORES) {
                
            // Colocamos el estado del DMA en 1 que indica error
            dma.estado = 1; 
            logger_log("DMA Error: Coordenadas invalidas (%d, %d, %d)\n", 
                dma.pista_seleccionada, dma.cilindro_seleccionado, dma.sector_seleccionado);
        } else {
            // Si las coordenadas estan bien colocamos el estado en 0 que indica exito
            dma.estado = 0;

            // Puntero al sector fisico
            Sector_t *sector = &disco.plato[dma.pista_seleccionada][dma.cilindro_seleccionado][dma.sector_seleccionado];
            int dir = dma.direccion_memoria;

            // Validamos que la dirrecion este en el rango permitido
            if (dir < 0 || dir >= TAMANO_MEMORIA) {
                    dma.estado = 1;
                    logger_log("DMA Error: Direccion de RAM invalida (%d)\n", dir);
            } else {
                if (dma.es_escritura == 1) { // 1 = Escribir (RAM -> Disco)
                    // Usamos snprintf para escribir dentro en el disco pasando de entero (RAM) a char(Disco)
                    snprintf(sector->datos, TAMANO_SECTOR, "%d", cpu_ptr->memoria[dir]);
                    logger_log("DMA Escritura: RAM[%d] (%d) -> Disco[%d][%d][%d]\n",
                        dir, cpu_ptr->memoria[dir], dma.pista_seleccionada, dma.cilindro_seleccionado, dma.sector_seleccionado);
                } else { // 0 = Leer (Disco -> RAM)
                    // Convertimos el string del sector a entero
                    int valor = atoi(sector->datos);
                    cpu_ptr->memoria[dir] = valor;
                    logger_log("DMA Lectura: Disco[%d][%d][%d] ('%s') -> RAM[%d] (%d)\n",
                        dma.pista_seleccionada, dma.cilindro_seleccionado, dma.sector_seleccionado, sector->datos, dir, valor);
                }
            }
        }

        // Usamos la bandera implementada en cpu.c para avisar al procesador
        if (cpu_ptr->interrupcion_pendiente == 0) {
            cpu_ptr->interrupcion_pendiente = 1;
            cpu_ptr->codigo_interrupcion = 4; // 4 = Fin de Entrada/Salida
        }

        dma.activo = 0; // Apagamos el DMA y liberamos el bus
        pthread_mutex_unlock(&cpu_ptr->mutex);
    }
    
    return NULL;
}