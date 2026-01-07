#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/cpu.h" 
#include "../include/constantes.h"
#define MAX_VALOR 99999999
#define MIN_VALOR -99999999

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

// Valida si una dirección física es legal para el proceso actual
// Retorna 1 si es válida, 0 si es ilegal (y dispara interrupción)
int validar_direccion(int dir_fisica) {
    // Si estamos en Modo Kernel (1), todo está permitido
    if (cpu.psw.modo_operacion == 1) {
        return 1;
    }

    // En Modo Usuario (0), verificamos los límites
    if (dir_fisica < cpu.RB || dir_fisica > cpu.RL) {
        printf("[INT] Violacion de segmento: Dir %d fuera de rango (%d-%d)\n", 
               dir_fisica, cpu.RB, cpu.RL);
        // Aquí deberíamos disparar la interrupción INT_DIR_INVALIDA
        return 0;
    }
    return 1;
}

// Resuelve el valor real del operando según el modo
// Retorna el valor listo para usar en sumas, cargas, etc.
// OJO: NO SIRVE PARA STR (Store), porque STR necesita una dirección, no un valor.
int obtener_valor_operando(int modo, int operando) {
    int valor = 0;
    int direccion_final = 0;

    switch (modo) {
        case DIR_INMEDIATO: // Modo 1
            valor = operando;
            break;
            
        case DIR_DIRECTO: // Modo 0
            // Calculamos dirección física
            direccion_final = operando + cpu.RB; 
            if (validar_direccion(direccion_final)) {
                valor = cpu.memoria[direccion_final];
            }
            break;
            
        case DIR_INDEXADO: // Modo 2 [cite: 41]
            // "índice a partir del acumulador" (Ojo al PDF, a veces dice RX o AC, verifiquemos)
            // El PDF dice en pág 3 pto 38: "índice a partir del acumulador"
            // PERO también existen registros RX. Usualmente Indexado es con RX.
            // Asumamos RX por lógica común, o AC si somos estrictos con el texto "a partir del acumulador".
            // Vamos a usar RX que es lo estándar para índices:
            direccion_final = operando + cpu.RX + cpu.RB;
             if (validar_direccion(direccion_final)) {
                valor = cpu.memoria[direccion_final];
            }
            break;
    }
    return valor;
}

int paso_cpu() {
    int instruccion, opcode, modo, operando;
    int val_operando; // Valor final para operar

    // --- 1. FETCH (Búsqueda) ---
    // a. MAR <- PC
    cpu.MAR = cpu.psw.pc;
    
    // b. Validar acceso a memoria (Fetch)
    if (!validar_direccion(cpu.MAR)) return 0;

    // c. MDR <- Memoria[MAR]
    cpu.MDR = cpu.memoria[cpu.MAR];

    // d. IR <- MDR
    cpu.IR = cpu.MDR;

    // e. PC++ (Apunta a la siguiente instrucción)
    cpu.psw.pc++;

    // --- 2. DECODE (Decodificación) ---
    // Formato: OPCODE (2) | MODO (1) | OPERANDO (5)
    instruccion = cpu.IR;
    
    // Matemáticas para separar los dígitos:
    opcode = (instruccion / 1000000);     // Los primeros 2 dígitos
    modo = (instruccion / 100000) % 10;   // El 3er dígito
    operando = (instruccion % 100000);    // Los últimos 5 dígitos
    
    // Debugging visual
    printf("[CPU] PC:%04d | IR:%08d -> OP:%02d M:%d VAL:%05d\n", 
        cpu.MAR, instruccion, opcode, modo, operando);
        
    // --- 3. EXECUTE (Ejecución) ---
    switch (opcode) {

        // --- ARITMÉTICA CON DETECCIÓN DE DESBORDAMIENTO (CC=3) ---
        case OP_SUM: // 00 - Sumar
        {
            // Lógica: AC = AC + Valor
            int val = obtener_valor_operando(modo, operando);
            long long resultado_temp = (long long)cpu.AC + val; // Usamos long long para ver si se pasa

            if (resultado_temp > MAX_VALOR || resultado_temp < MIN_VALOR) {
                cpu.psw.codigo_condicion = 3;
            } else {
                cpu.AC = (int)resultado_temp;
                // Actualizamos CC normal (Opcional, algunos CPUs lo hacen)
                if (cpu.AC == 0) cpu.psw.codigo_condicion = 0;
                else if (cpu.AC < 0) cpu.psw.codigo_condicion = 1;
                else cpu.psw.codigo_condicion = 2;
            }
            break;
        }    

        case OP_RES: // 01 - Restar
        {
            // Lógica: AC = AC - Valor
            int val = obtener_valor_operando(modo, operando);
            long long resultado_temp = (long long)cpu.AC - val;

            if (resultado_temp > MAX_VALOR || resultado_temp < MIN_VALOR) {
                cpu.psw.codigo_condicion = 3;
            } else {
                cpu.AC = (int)resultado_temp;
                // Actualizar CC normal
                if (cpu.AC == 0) cpu.psw.codigo_condicion = 0;
                else if (cpu.AC < 0) cpu.psw.codigo_condicion = 1;
                else cpu.psw.codigo_condicion = 2;
            }
            break;
        }
            
        case OP_MULT: // 02 - Multiplicar
        {
            // Lógica: AC = AC * Valor
            int val = obtener_valor_operando(modo, operando);
            long long resultado_temp = (long long)cpu.AC * val;

            if (resultado_temp > MAX_VALOR || resultado_temp < MIN_VALOR) {
                cpu.psw.codigo_condicion = 3;
            } else {
                cpu.AC = (int)resultado_temp;
                if (cpu.AC == 0) cpu.psw.codigo_condicion = 0;
                else if (cpu.AC < 0) cpu.psw.codigo_condicion = 1;
                else cpu.psw.codigo_condicion = 2;
            }
            break;
        }

        case OP_DIVI: // 03 - Dividir
        {
            // Lógica: AC = AC / Valor
            int val = obtener_valor_operando(modo, operando);
            if (val != 0) {
                cpu.AC /= val;
                if (cpu.AC == 0) cpu.psw.codigo_condicion = 0;
                else if (cpu.AC < 0) cpu.psw.codigo_condicion = 1;
                else cpu.psw.codigo_condicion = 2;
            } else {
                cpu.psw.codigo_condicion = 3; // Podemos usar el 3 también para Error Matemático
                printf("[ERROR] Division por cero en PC=%d\n", cpu.psw.pc);
                return 0;
            }
            break;
        }
            
        case OP_LOAD: // 04 - Cargar en AC
        {
            // Lógica: AC = Valor
            cpu.AC = obtener_valor_operando(modo, operando);
            break;
        }
            
        case OP_STR: // 05 - Guardar AC en Memoria
        {
            // Lógica: Memoria[Operando] = AC
            int dir_destino = -1;
            if (modo == DIR_DIRECTO) {
                dir_destino = operando + cpu.RB;
            } else if (modo == DIR_INDEXADO) {
                dir_destino = operando + cpu.RB + cpu.RX;
            }

            // Solo escribimos si la dirección es válida (y no es -1)
            if (dir_destino != -1 && validar_direccion(dir_destino)) {
                cpu.memoria[dir_destino] = cpu.AC;
                printf("      -> Guardado %d en Mem[%d]\n", cpu.AC, dir_destino);
            }
            break;
        }

        case OP_LOADRX: // 06 - Cargar en RX
        {
            // Lógica: RX = Valor
            cpu.RX = obtener_valor_operando(modo, operando);
            break;
        }

        case OP_STRRX: // 07 - Guardar RX en Memoria
        {
            // Igual que STR, pero la fuente es RX
            int dir_destino = -1;
            if (modo == DIR_DIRECTO) {
                dir_destino = operando + cpu.RB;
            } else if (modo == DIR_INDEXADO) {
                dir_destino = operando + cpu.RB + cpu.RX;
            }

            if (dir_destino != -1 && validar_direccion(dir_destino)) {
                cpu.memoria[dir_destino] = cpu.RX;
                printf("      -> Guardado RX (%d) en Mem[%d]\n", cpu.RX, dir_destino);
                }
            break;
        }

        case OP_COMP: // 08 - Comparar
        {
            int val = obtener_valor_operando(modo, operando);
            if (cpu.AC == val) {
                cpu.psw.codigo_condicion = 0; // Iguales
            } else if (cpu.AC < val) {
                cpu.psw.codigo_condicion = 1; // Menor
            } else {
                cpu.psw.codigo_condicion = 2; // Mayor
            }
            break;
        }

        // --- SALTOS (JUMPS) ---
        case OP_JMPE: // 09 - Jump Equal (Si CC == 0)
        {
            if (cpu.psw.codigo_condicion == 0) {
                cpu.psw.pc = cpu.RB + operando;
            }
            break;
        }

        case OP_JMPNE: // 10 - Jump Not Equal (Si CC != 0)
        {
            if (cpu.psw.codigo_condicion != 0) {
                cpu.psw.pc = cpu.RB + operando;
            }
            break;
        }

        case OP_JMPLT: // 11 - Jump Less Than (Si CC == 1)
        {
            if (cpu.psw.codigo_condicion == 1) {
                cpu.psw.pc = cpu.RB + operando;
            }
            break;
        }
            
        case OP_JMPLGT: // 12 - Jump Greater Than (Si CC == 2)
        {
            if (cpu.psw.codigo_condicion == 2) {
                cpu.psw.pc = cpu.RB + operando;
            }
            break;
        }

        case OP_J: // 27 - Salto Incondicional (Salta siempre)
        {
            cpu.psw.pc = cpu.RB + operando;
            break;
        }
        
        case OP_SVC: // 13 - System Call (Terminar por ahora)
        {
            printf("[CPU] SVC detectado (Fin del programa o llamada al sistema)\n");
            return 0; // Detenemos la CPU
            break;
        }

        default:
        {
            printf("[ERROR] Opcode %d no implementado aun.\n", opcode);
            return 0;
        }
    }
    
    return 1; // Continuar ejecutando
}

void ejecutar_cpu() {
    printf("--- INICIANDO EJECUCION ---\n");
    while (cpu.ejecutando) {
        if (!paso_cpu()) {
            cpu.ejecutando = 0; // Detener si paso_cpu retorna 0
        }
        dump_cpu();
        // Aquí podríamos poner un sleep() si fuera visualización lenta
    }
    printf("--- EJECUCION FINALIZADA ---\n");
}