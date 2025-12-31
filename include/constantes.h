#ifndef CONSTANTES_H
#define CONSTANTES_H

// --- CONFIGURACIÓN GENERAL ---
#define TAMANO_MEMORIA 2000     //
#define TAMANO_PALABRA 8        // 8 dígitos
#define INICIO_SO      0        // Inicio memoria SO
#define FIN_SO         299      // Las primeras 300 son del SO
#define INICIO_USUARIO 300      // El usuario empieza en la 300

// --- CÓDIGOS DE OPERACIÓN (OpCodes) ---
// Aritméticas
#define OP_SUM     0
#define OP_RES     1
#define OP_MULT    2
#define OP_DIVI    3
// Transferencia de Datos
#define OP_LOAD    4
#define OP_STR     5
#define OP_LOADRX  6
#define OP_STRRX   7
// Comparación y Saltos
#define OP_COMP    8
#define OP_JMPE    9   // Jump if Equal
#define OP_JMPNE   10  // Jump if Not Equal
#define OP_JMPLT   11  // Jump if Less Than
#define OP_JMPLGT  12  // Jump if Greater Than
// Sistema y Control
#define OP_SVC     13  // System Call
#define OP_RETRN   14
#define OP_HAB     15  // Habilitar interrupciones
#define OP_DHAB    16  // Deshabilitar interrupciones
#define OP_TTI     17  // Timer
#define OP_CHMOD   18  // Cambiar modo
// Protección (Registros Base/Limite)
#define OP_LOADRB  19
#define OP_STRRB   20
#define OP_LOADRL  21
#define OP_STRRL   22
// Pila (Stack)
#define OP_LOADSP  23
#define OP_STRSP   24
#define OP_PSH     25  // Push
#define OP_POP     26  // Pop
// Salto Incondicional
#define OP_J       27
// Entrada/Salida (DMA)
#define OP_SDMAP   28  // Set DMA Pista
#define OP_SDMAC   29  // Set DMA Cilindro
#define OP_SDMAS   30  // Set DMA Sector
#define OP_SDMAIO  31  // Set DMA I/O (Leer/Escribir)
#define OP_SDMAM   32  // Set DMA Memoria Direccion
#define OP_SDMAON  33  // Encender DMA

// --- CÓDIGOS DE INTERRUPCIÓN ---
#define INT_SVC_INVALIDO 0
#define INT_COD_INVALIDO 1
#define INT_SYSCALL      2
#define INT_RELOJ        3
#define INT_IO_FIN       4
#define INT_INST_ILLEGAL 5
#define INT_DIR_INVALIDA 6
#define INT_UNDERFLOW    7
#define INT_OVERFLOW     8

// --- MODOS DE DIRECCIONAMIENTO ---
#define DIR_DIRECTO    0
#define DIR_INMEDIATO  1
#define DIR_INDEXADO   2

#endif