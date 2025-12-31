#include <stdio.h>
#include <string.h> 
#include "../include/cpu.h" 
#include "../include/constantes.h"

// 1. Instanciamos la variable global real (La máquina)
CPU_t cpu;

void inicializar_cpu() {
    // Limpiar toda la memoria y registros a 0
    memset(&cpu, 0, sizeof(CPU_t));

    // Configuración inicial según PDF
    cpu.psw.modo_operacion = 1;      // Arranca en modo Kernel (1)
    cpu.psw.interrupciones = 0;      // Deshabilitadas al inicio
    cpu.psw.codigo_condicion = 0;
    
    // El PC debe apuntar a donde arrancará el programa.
    // Por ahora, lo ponemos en el inicio de usuario.
    cpu.psw.pc = INICIO_USUARIO;     
    
    // Inicializar registros de protección (Todo el espacio de usuario)
    cpu.RB = INICIO_USUARIO;
    cpu.RL = TAMANO_MEMORIA - 1;

    // Bandera para el bucle principal
    cpu.ejecutando = 1;

    printf("[INFO] CPU Inicializada. Modo Kernel. Memoria limpia (0-1999).\n");
}

void dump_cpu() {
    // Esta función nos servirá para ver qué pasa dentro (Debugger)
    printf("\n=== ESTADO CPU ===\n");
    printf("PC: %04d | IR: %08d | AC: %08d\n", cpu.psw.pc, cpu.IR, cpu.AC);
    printf("MAR: %04d | MDR: %08d\n", cpu.MAR, cpu.MDR);
    printf("PSW: CC=%d Mode=%d Int=%d\n", 
           cpu.psw.codigo_condicion, cpu.psw.modo_operacion, cpu.psw.interrupciones);
    printf("Stack: SP=%d RX=%d\n", cpu.SP, cpu.RX);
    printf("==================\n");
}