#ifndef CONSTANTES_H
#define CONSTANTES_H

// Configuracion general de la maquina virtual
#define TAMANO_MEMORIA 2000     // 2000 espacios para almacenar instrucciones o datos en memoria
#define TAMANO_PALABRA 8        // 8 dígitos
#define INICIO_SO      0        // Inicio de la memoria del sistema operativo 
#define FIN_SO         299      // Fin de la memoria del sistema operativo
#define INICIO_USUARIO 300      // La memoria del usuario empieza en la 300 y termina en la 1999

// 1. Constantes definidas para cada codigo de operacion
// Aritmeticas
#define OP_SUM     0
#define OP_RES     1
#define OP_MULT    2
#define OP_DIVI    3
// Manipulacion de datos
#define OP_LOAD    4
#define OP_STR     5
#define OP_LOADRX  6
#define OP_STRRX   7
// Comparaciones y saltos
#define OP_COMP    8
#define OP_JMPE    9
#define OP_JMPNE   10
#define OP_JMPLT   11
#define OP_JMPLGT  12
// Sistema y control
#define OP_SVC     13
#define OP_RETRN   14
#define OP_HAB     15
#define OP_DHAB    16
#define OP_TTI     17
#define OP_CHMOD   18
// Registros base y limite
#define OP_LOADRB  19
#define OP_STRRB   20
#define OP_LOADRL  21
#define OP_STRRL   22
// Manipulacion de la pila
#define OP_LOADSP  23
#define OP_STRSP   24
#define OP_PSH     25
#define OP_POP     26
// Salto incondicional
#define OP_J       27
// Entrada y salida (DMA)
#define OP_SDMAP   28
#define OP_SDMAC   29
#define OP_SDMAS   30
#define OP_SDMAIO  31
#define OP_SDMAM   32
#define OP_SDMAON  33

// 2. Codigos de interrupciones
#define INT_SVC_INVALIDO 0
#define INT_COD_INVALIDO 1
#define INT_SYSCALL      2
#define INT_RELOJ        3
#define INT_IO_FIN       4
#define INT_INST_ILLEGAL 5
#define INT_DIR_INVALIDA 6
#define INT_UNDERFLOW    7
#define INT_OVERFLOW     8

// 3. Modos de direccionamiento
#define DIR_DIRECTO    0
#define DIR_INMEDIATO  1
#define DIR_INDEXADO   2

#endif