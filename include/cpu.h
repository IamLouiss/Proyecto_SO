#ifndef CPU_H
#define CPU_H

#include "constantes.h"
#include <pthread.h>

// Estructura para la Palabra de Estado del Programa (PSW)
typedef struct {
    int codigo_condicion; // Resultado de la ultima operacion: 0(=), 1(<), 2(>), 3(Overflow)
    int modo_operacion;   // 0 = usuario, 1 = kernel
    int interrupciones;   // 0 = deshabilitadas, 1 = habilitadas
    int pc;               // Contador de programa que indica la proxima instruccion a ejecutar
} PSW_t;

// Estructura principal de la CPU
typedef struct {
    // Registros de proposito especial
    int AC;   // Acumulador (Donde se guardan valores para operar)
    int MAR;  // Memory Address Register (Direccion a buscar)
    int MDR;  // Memory Data Register (Dato leido de la memoria)
    int IR;   // Instruction Register (Instruccion actual que se va a ejecutar)
    
    // Registros de proteccion de memoria
    int RB;   // Registro base (Inicio del proceso)
    int RL;   // Registro limite (Final de la memoria del proceso)
    
    // Registros de pila
    int RX;   // Registro base de la pila
    int SP;   // Puntero al tope de la pila

    // Hilos y timer
    int timer_periodo;          // Cada cuantos ciclos debe sonar el timer
    int interrupcion_pendiente; // Bandera para saber si hay alguna interrupcion pendiente
    int codigo_interrupcion;    // Codigo para saber que tipo de interrupcion ocurrio

    // Control de hilos
    pthread_mutex_t mutex;    // Candado para evitar que la CPU o DMA toquen la ram al mismo tiempo
    pthread_cond_t  cond_dma; // variable de condicion para despertar al DMA
    
    // Palabra de estado
    PSW_t psw;

    // Memoria principal (RAM)
    int memoria[TAMANO_MEMORIA]; // Simula la RAM fisica

    // Variable bandera para saber si la CPU debe seguir corriendo
    int ejecutando;

} CPU_t;

// Variable global que representa nuestra maquina unica
extern CPU_t cpu;

// -- Prototipo de funciones:

// Inicializa el cpu con valores por defecto
void inicializar_cpu();

// Debugger del cpu para mostrar el estado de los registros
void dump_cpu();

// Ejecuta una instruccion (Fetch -> Decode -> Execute)
// Retorna 1 si salio bien y 0 si hubo error o Halt
int paso_cpu();

// Bucle principal que llama a paso_cpu hasta terminar
void ejecutar_cpu(int modo_debug);

// Funcion que corre el paralelo usando hilos para contar el tiempo
// y avisar a la CPU cuando se termina el reloj (timer)
void *hilo_timer(void *arg);

#endif