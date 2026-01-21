#ifndef DISCO_H
#define DISCO_H

#include "constantes.h"

// Definiciones fisicas del disco
#define DISCO_PISTAS    10  // Numero de pistas
#define DISCO_CILINDROS 10  // Numero de cilindros
#define DISCO_SECTORES  100 // Numero de sectores
#define TAMANO_SECTOR   9   // Cada sector tiene un dato de 9 caracteres

// Estructura de un sector
typedef struct {
    char datos[TAMANO_SECTOR]; 
} Sector_t;

// Estructura del disco duro completo
typedef struct {
    Sector_t plato[DISCO_PISTAS][DISCO_CILINDROS][DISCO_SECTORES];
} Disco_t;

// Controlador del DMA
typedef struct {
    // Registros de configuracion (Usa las instrucciones SDMAP, SDMAC y SDMAS)
    int pista_seleccionada;    // Se le asigna la pista a leer/escribir con la instruccion SDMAP
    int cilindro_seleccionado; // Se le asigna el cilindro a leer/escribir con la instruccion SDMAC
    int sector_seleccionado;   // Se le asigna el sector a leer/escribir con la instruccion SDMAS
    int direccion_memoria;     // A donde va/viene el dato en RAM
    
    int es_escritura;      // Bandera para el tipo de operacion: 0 = Lectura (Disco -> RAM), 1 = Escritura (RAM -> Disco)
    int estado;            // Registro para el estado del DMA: 0 = Exito, 1 = Error
    int activo;            // Bandera para determinar el funcionamiento del DMA: 1 = Trabajando, 0 = Inactivo
} DMA_t;

// Variables globales
extern Disco_t disco;
extern DMA_t dma;

// Funciones

// Limpia el disco y resetea el DMA
void inicializar_disco();

// Funcion del hilo que simula el trabajo mecanico del disco
// Corre en paralelo a la CPU esperando ordenes
void *hilo_dma(void *arg);

#endif