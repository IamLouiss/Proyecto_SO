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
    
    //********************************************************************** *
    
    // Inicializar registros de protección (Todo el espacio de usuario)
    cpu.RB = INICIO_USUARIO;
    cpu.RL = TAMANO_MEMORIA - 1;
    cpu.SP = cpu.RL - cpu.RB;
    
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
    // Agregamos RB (Base) y RL (Limite) para ver la "cancha" de memoria
    printf("Stack: SP=%d | RB=%d | RLLLLL=%d\n", cpu.SP, cpu.RB, cpu.RL);
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
     

        // --- SISTEMA Y CONTROL (13-18) ---
        
        case OP_SVC: // Código 13: System Call (Llamada al Sistema)                                   FFIIIIIXXXX TTTHHHIIISSS!!!!!
            // Se usa para solicitar servicios al Kernel (como E/S o terminar).
            // En esta Fase 1, si AC=0 asumimos que se pide terminar la simulación.
            printf("      -> [SVC] Llamada al sistema detectada. Codigo en AC: %d\n", cpu.AC);
            if (cpu.AC == 0) {
                printf("      -> [INFO] SVC 0: Solicitud de fin de programa.\n");
                cpu.ejecutando = 0; // Detiene el bucle principal
            }
            break;

        case OP_RETRN: // Código 14: Return (Retorno de Subrutina)                                   FFIIIIIXXXX TTTHHHIIISSS!!!!!
            // Recupera el valor del PC que estaba guardado en el tope de la Pila.
            // Esto permite volver al lugar donde se llamó a la función.
            if (cpu.SP < TAMANO_MEMORIA) {
                cpu.psw.pc = cpu.memoria[cpu.SP]; // Leemos la dirección de retorno
                cpu.SP++; // "Desapilamos" (incrementamos SP)
                printf("      -> [RETRN] Retornando a la direccion %d\n", cpu.psw.pc);
            } else {
                printf("      -> [ERROR] Stack Underflow al intentar RETRN.\n");
                cpu.ejecutando = 0;
            }
            break;

        case OP_HAB: // Código 15: Habilitar Interrupciones
            cpu.psw.interrupciones = 1;
            printf("      -> [HAB] Interrupciones HABILITADAS.\n");
            break;

        case OP_DHAB: // Código 16: Deshabilitar Interrupciones
            cpu.psw.interrupciones = 0;
            printf("      -> [DHAB] Interrupciones DESHABILITADAS.\n");
            break;

        case OP_TTI: // Código 17: Timer Interrupt Time
            // Configura el temporizador del sistema (simulado).
            printf("      -> [TTI] Intervalo del reloj configurado a %d ciclos.\n", operando);
            break;

        case OP_CHMOD: // Código 18: Change Mode
            // Cambia el modo de ejecución: 0 = Usuario, 1 = Kernel.
            cpu.psw.modo_operacion = operando;
            printf("      -> [CHMOD] Modo cambiado a: %s\n", (operando ? "KERNEL" : "USUARIO"));
            break;

        // --- PROTECCIÓN DE MEMORIA (19-22) ---
        // Estos registros delimitan qué parte de la memoria puede usar el programa actual.

        case OP_LOADRB: // 19: Cargar Registro Base
            cpu.AC = cpu.RB; 
            printf("      -> [LOADRB] AC cargado con RB (%d)\n", cpu.RB);
            break;
            
        case OP_STRRB:  // 20: Guardar en Registro Base
            cpu.RB = cpu.AC; 
            printf("      -> [STRRB] RB actualizado con AC (%d)\n", cpu.RB);
            break;

        case OP_LOADRL: // 21: Cargar Registro Límite
            cpu.AC = cpu.RL; 
            printf("      -> [LOADRL] AC cargado con RL (%d)\n", cpu.RL);
            break;

        case OP_STRRL:  // 22: Guardar en Registro Límite
            cpu.RL = cpu.AC; 
            printf("      -> [STRRL] RL actualizado con AC (%d)\n", cpu.RL);
            break;

        // --- MANEJO DE PILA (STACK) (23-26) ---
        
        case OP_LOADSP: // 23: Cargar Stack Pointer a AC
            cpu.AC = cpu.SP;
            printf("      -> [LOADSP] AC cargado con SP (%d)\n", cpu.AC);
            break;

        case OP_STRSP:  // 24: Actualizar Stack Pointer desde AC
            cpu.SP = cpu.AC;
             printf("      -> [STRSP] SP actualizado con AC (%d)\n", cpu.RL);
            break;

        case OP_PSH: // 25: PUSH
        
        // 1. Calculamos la dirección física REAL
        int dir_fisica = cpu.RB + cpu.SP;
        
        // 2. Verificamos seguridad
        //    (SP >= 0) asegura que no bajemos más allá del piso 0 relativo
        if (cpu.SP >= 0 && dir_fisica < TAMANO_MEMORIA) {

            cpu.memoria[dir_fisica] = cpu.AC; 
            printf("      -> [PSH] Valor %d apilado en MemFisica[%d] (SP Logico: %d)\n", 
                cpu.AC, dir_fisica, cpu.SP);
                // 3. RESTAMOS Para pasar de 1700 (imaginario) a 1699 (real)
                cpu.SP--;
        } else {
            printf("      -> [ERROR] Stack Overflow (Fisica: %d)\n", dir_fisica);
            cpu.ejecutando = 0;
        }
        break;

        case OP_POP: // 26: POP (Desapilar)

            // 1. VALIDAR SI HAY DATOS (Stack Underflow)
            // Tu tope inicial calculado es (cpu.RL - cpu.RB) = 1699.
            // Si SP = 1699, significa que no hemos hecho ningún PUSH todavía.
            int tope = TAMANO_MEMORIA - INICIO_USUARIO -1;
            if (cpu.SP < tope) { 
                
                // 2. SUMAR PRIMERO (Pre-incremento)
                // Pasamos de la posición vacía (ej. 1698) a la llena (1699)
                cpu.SP++; 
                
                // 3. CALCULAR DIRECCIÓN FÍSICA
                int dir_fisica_pop = cpu.RB + cpu.SP;

                // 4. LEER EL DATO
                cpu.AC = cpu.memoria[dir_fisica_pop];
                
                printf("      -> [POP] Recuperado %d de MemFisica[%d] (SP Logico: %d)\n", 
                cpu.AC, dir_fisica_pop, cpu.SP);
                       
            } else {
                printf("      -> [ERROR] Stack Underflow (La pila esta vacia)\n");
                cpu.ejecutando = 0;
            }
            break;

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