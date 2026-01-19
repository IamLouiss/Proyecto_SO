#ifndef DISCO_H
#define DISCO_H

#include "constantes.h"

// --- DEFINICIONES FÍSICAS DEL DISCO ---
#define DISCO_PISTAS    10
#define DISCO_CILINDROS 10
#define DISCO_SECTORES  100
#define TAMANO_SECTOR   9   // "Cada sector tiene un dato de 9 caracteres"

// Estructura de un Sector (La unidad mínima de almacenamiento)
typedef struct {
    char datos[TAMANO_SECTOR]; 
} Sector_t;

// El Disco Duro completo (Matriz 3D)
typedef struct {
    Sector_t plato[DISCO_PISTAS][DISCO_CILINDROS][DISCO_SECTORES];
} Disco_t;

// --- CONTROLADOR DMA (El intermediario) ---
typedef struct {
    // Registros de configuración (Llenados por instrucciones SDMAP, SDMAC, etc.)
    int pista_seleccionada;
    int cilindro_seleccionado;
    int sector_seleccionado;
    int direccion_memoria; // A dónde va/viene el dato en RAM
    
    int es_escritura;      // 0 = Leer del Disco (Disk->RAM), 1 = Escribir (RAM->Disk)
    int estado;            // Registro ESTADOdma (0=Exito, 1=Error)
    int activo;            // 1 = Trabajando, 0 = Inactivo
} DMA_t;

// Variables Globales (Para que cpu.c y main.c las vean)
extern Disco_t disco;
extern DMA_t dma;

// Funciones
void inicializar_disco();
void *hilo_dma(void *arg); // El hilo que moverá los datos

#endif