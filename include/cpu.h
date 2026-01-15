#ifndef CPU_H
#define CPU_H

#include "constantes.h"
#include <pthread.h>

// Estructura para la Palabra de Estado del Programa (PSW)
typedef struct {
    int codigo_condicion; // 0(=), 1(<), 2(>), 3(Overflow)
    int modo_operacion;   // 0=Usuario, 1=Kernel
    int interrupciones;   // 0=Deshabilitadas, 1=Habilitadas
    int pc;               // Program Counter (Próxima instrucción)
} PSW_t;

// Estructura Principal de la CPU
typedef struct {
    // Registros de Propósito Especial
    int AC;   // Acumulador (Para operaciones aritméticas)
    int MAR;  // Memory Address Register (Dirección a buscar)
    int MDR;  // Memory Data Register (Dato leído/a escribir)
    int IR;   // Instruction Register (Instrucción actual)
    
    // Registros de Protección de Memoria
    int RB;   // Registro Base (Inicio del proceso)
    int RL;   // Registro Límite (Tamaño del proceso)
    
    // Registros de Pila
    int RX;   // Registro Índice/Auxiliar
    int SP;   // Stack Pointer (Tope de la pila)

    // Hilos y timer
    int timer_periodo;
    int interrupcion_pendiente;
    int codigo_interrupcion;

    // Control de hilos
    pthread_mutex_t mutex;
    
    // Palabra de Estado
    PSW_t psw;

    // Memoria Principal (RAM)
    int memoria[TAMANO_MEMORIA]; 

    // Variable para saber si la CPU debe seguir corriendo
    int ejecutando; 

} CPU_t;

// Variable global que representa nuestra máquina única
extern CPU_t cpu;

// --- PROTOTIPOS DE FUNCIONES ---

// Inicializa el cpu con valores por defecto
void inicializar_cpu();

// Debugger del cpu
void dump_cpu();

// Ejecuta una instruccion (Fetch -> Decode -> Execute).
// Retorna 1 si salio bien y 0 si hubo error o Halt
int paso_cpu();

// Bucle principal que llama a paso_cpu hasta terminar
void ejecutar_cpu();

void *hilo_timer(void *arg);

#endif // CPU_H