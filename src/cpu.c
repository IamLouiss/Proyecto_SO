#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "../include/cpu.h" 
#include "../include/constantes.h"
#include "../include/logger.h"
#include "../include/disco.h"
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

    logger_log("[INFO] CPU Inicializada. Modo Kernel. Memoria limpia (0-1999).\n");
}

void dump_cpu() {
    // Esta función nos servirá para ver qué pasa dentro (Debugger)
    logger_log("\n=== ESTADO CPU ===\n");
    logger_log("PC: %04d | IR: %08d | AC: %08d\n", cpu.psw.pc, cpu.IR, cpu.AC);
    logger_log("MAR: %04d | MDR: %08d\n", cpu.MAR, cpu.MDR);
    logger_log("PSW: CC=%d Mode=%d Int=%d\n", 
           cpu.psw.codigo_condicion, cpu.psw.modo_operacion, cpu.psw.interrupciones);
    logger_log("Stack: SP=%d RX=%d\n", cpu.SP, cpu.RX);
    // Agregamos RB (Base) y RL (Limite) para ver la "cancha" de memoria
    logger_log("Stack: SP=%d | RB=%d | RL=%d\n", cpu.SP, cpu.RB, cpu.RL);
    logger_log("==================\n");
}

// Valida si una dirección física es legal para el proceso actual
// Retorna 1 si es válida, 0 si es ilegal (y dispara interrupción)
int validar_direccion(int dir_fisica) {
    // Si estamos en Modo Kernel (1), todo está permitido
    if (cpu.psw.modo_operacion == 1) {
        return 1;
    }

    // En Modo Usuario (0), verificamos los límites
    if (dir_fisica < cpu.RB || dir_fisica >= cpu.RL) {
        logger_log("[INT] Violacion de segmento: Dir %d fuera de rango (%d-%d)\n", 
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
    logger_log("[CPU] PC:%04d | IR:%08d -> OP:%02d M:%d VAL:%05d\n", 
        cpu.MAR, instruccion, opcode, modo, operando);
        
    // --- 3. EXECUTE (Ejecución) ---
    switch (opcode) {

        // --- ARITMÉTICA CON DETECCIÓN DE DESBORDAMIENTO (CC=3) ---
        case OP_SUM: // 00 - Sumar
        {
            // Lógica: AC = AC + Valor
            int val = obtener_valor_operando(modo, operando);
            long long resultado_temp = (long long)cpu.AC + val; // Usamos long long para ver si se pasa

            if (resultado_temp > MAX_VALOR) {
                cpu.interrupcion_pendiente = 1;
                cpu.codigo_interrupcion = 8;
                cpu.psw.codigo_condicion = 3;
            } else if (resultado_temp < MIN_VALOR) {
                cpu.interrupcion_pendiente = 1;
                cpu.codigo_interrupcion = 7;
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

            if (resultado_temp > MAX_VALOR) {
                cpu.interrupcion_pendiente = 1;
                cpu.codigo_interrupcion = 8;
                cpu.psw.codigo_condicion = 3;
            } else if (resultado_temp < MIN_VALOR) {
                cpu.interrupcion_pendiente = 1;
                cpu.codigo_interrupcion = 7;
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

            if (resultado_temp > MAX_VALOR) {
                cpu.interrupcion_pendiente = 1;
                cpu.codigo_interrupcion = 8;
                cpu.psw.codigo_condicion = 3;
            } else if (resultado_temp < MIN_VALOR) {
                cpu.interrupcion_pendiente = 1;
                cpu.codigo_interrupcion = 7;
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
                cpu.interrupcion_pendiente = 1;
                cpu.codigo_interrupcion = 8;
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
                logger_log("      -> Guardado %d en Mem[%d]\n", cpu.AC, dir_destino);
            } else {
                cpu.interrupcion_pendiente = 1;
                cpu.codigo_interrupcion = 6;
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
                logger_log("      -> Guardado RX (%d) en Mem[%d]\n", cpu.RX, dir_destino);
            } else {
                cpu.interrupcion_pendiente = 1;
                cpu.codigo_interrupcion = 6;
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

        // --- SISTEMA Y CONTROL (13-18) ---
        
        case OP_SVC: // Código 13: System Call (Llamada al Sistema)
        {
            // Se usa para solicitar servicios al Kernel (como E/S o terminar).
            // En esta Fase 1, si AC=0 asumimos que se pide terminar la simulación.
            cpu.interrupcion_pendiente = 1;
            cpu.codigo_interrupcion = 2; // Llamada al sistema

            logger_log("      -> [SVC] Llamada al sistema detectada. Codigo en AC: %d\n", cpu.AC);
            if (cpu.AC == 0) {
                logger_log("      -> [INFO] SVC 0: Solicitud de fin de programa.\n");
                cpu.ejecutando = 0; // Detiene el bucle principal
            }
            break;
        }
        
        case OP_RETRN: 
        {
            // Código 14: Return (Retorno de Subrutina)
            // Recupera el valor del PC que estaba guardado en el tope de la Pila.
            // Esto permite volver al lugar donde se llamó a la función.
            int tope = (cpu.RL - cpu.RB) - 1;
            if (cpu.SP < tope) {
                cpu.SP++; // Pasamos de la posicion vacia a la llena
                cpu.psw.pc = cpu.memoria[cpu.SP + cpu.RB]; // Leemos la dirección de retorno
                logger_log("      -> [RETRN] Retornando a la direccion %d (Stack[%d])\n", cpu.psw.pc, cpu.SP);
            } else {
                cpu.interrupcion_pendiente = 1;
                cpu.codigo_interrupcion = 7;
            }
            break;
        }
        
        case OP_HAB: // Código 15: Habilitar Interrupciones
        {
            cpu.psw.interrupciones = 1;
            logger_log("      -> [HAB] Interrupciones HABILITADAS.\n");
            break;
        }
        
        case OP_DHAB: // Código 16: Deshabilitar Interrupciones
        {
            cpu.psw.interrupciones = 0;
            logger_log("      -> [DHAB] Interrupciones DESHABILITADAS.\n");
            break;
        }
        
        case OP_TTI: // 17 - Configurar Timer
        {
            // Ahora sí guardamos el valor para que el hilo lo lea
            pthread_mutex_lock(&cpu.mutex); // Protegemos el cambio
            cpu.timer_periodo = operando;
            pthread_mutex_unlock(&cpu.mutex);
            
            logger_log("      -> [TTI] Timer configurado a %d ciclos (aprox %d ms).\n", 
                operando, operando * 10);
            break;
        }
            
        case OP_CHMOD: // 18
        {
            if (cpu.psw.modo_operacion == 0) {
                // [PDF] Privilegio -> Instrucción Inválida (5) o podemos definir una nueva.
                // Usaremos 5 (Instrucción Invalida para este modo)
                cpu.interrupcion_pendiente = 1;
                cpu.codigo_interrupcion = 5; 
                logger_log("      -> [ERROR] Violacion de Privilegios\n");
            } else if (operando == 0 || operando == 1) {
                cpu.psw.modo_operacion = operando;
                logger_log("      -> [CHMOD] Modo: %d\n", operando);
            } else {
                cpu.interrupcion_pendiente = 1;
                cpu.codigo_interrupcion = 5; // Argumento invalido
            }
            break;
        }
            
        // --- PROTECCIÓN DE MEMORIA (19-22) ---
        // Estos registros delimitan qué parte de la memoria puede usar el programa actual.
            
        case OP_LOADRB: // 19: Cargar Registro Base
        {
            cpu.AC = cpu.RB; 
            logger_log("      -> [LOADRB] AC cargado con RB (%d)\n", cpu.RB);
            break;
        }
        
        case OP_STRRB:  // 20: Guardar en Registro Base
        {
            cpu.RB = cpu.AC; 
            logger_log("      -> [STRRB] RB actualizado con AC (%d)\n", cpu.RB);
            break;
        }
        
        case OP_LOADRL: // 21: Cargar Registro Límite
        {
            cpu.AC = cpu.RL; 
            logger_log("      -> [LOADRL] AC cargado con RL (%d)\n", cpu.RL);
            break;
        }
        
        case OP_STRRL:  // 22: Guardar en Registro Límite
        {
            cpu.RL = cpu.AC; 
            logger_log("      -> [STRRL] RL actualizado con AC (%d)\n", cpu.RL);
            break;
        }
        
        // --- MANEJO DE PILA (STACK) (23-26) ---
        
        case OP_LOADSP: // 23: Cargar Stack Pointer a AC
        {
            cpu.AC = cpu.SP;
            logger_log("      -> [LOADSP] AC cargado con SP (%d)\n", cpu.AC);
            break;
        }
        
        case OP_STRSP:  // 24: Actualizar Stack Pointer desde AC
        {
            cpu.SP = cpu.AC;
            logger_log("      -> [STRSP] SP actualizado con AC (%d)\n", cpu.SP);
            break;
        }
        
        case OP_PSH: // 25: PUSH
        {
            // 1. Calculamos la dirección física REAL
            int dir_fisica = cpu.RB + cpu.SP;
            
            // 2. Verificamos seguridad
            //    (SP >= 0) asegura que no bajemos más allá del piso 0 relativo
            if (cpu.SP >= 0 && dir_fisica < cpu.RL) {
                
                cpu.memoria[dir_fisica] = cpu.AC; 
                logger_log("      -> [PSH] Valor %d apilado en MemFisica[%d] (SP Logico: %d)\n", 
                cpu.AC, dir_fisica, cpu.SP);
                // 3. RESTAMOS Para pasar de 1700 (imaginario) a 1699 (real)
                cpu.SP--;
            } else {
                cpu.interrupcion_pendiente = 1;
                cpu.codigo_interrupcion = 8;
            }
            break;
        }
        
        case OP_POP: // 26: POP (Desapilar)
        {
            // 1. VALIDAR SI HAY DATOS (Stack Underflow)
            // Tu tope inicial calculado es (cpu.RL - cpu.RB) = 1699.
            // Si SP = 1699, significa que no hemos hecho ningún PUSH todavía.
            int tope = (cpu.RL - cpu.RB) - 1;;
            if (cpu.SP < tope) { 
                
                // 2. SUMAR PRIMERO (Pre-incremento)
                // Pasamos de la posición vacía (ej. 1698) a la llena (1699)
                cpu.SP++; 
                
                // 3. CALCULAR DIRECCIÓN FÍSICA
                int dir_fisica_pop = cpu.RB + cpu.SP;
                
                // 4. LEER EL DATO
                cpu.AC = cpu.memoria[dir_fisica_pop];
                
                logger_log("      -> [POP] Recuperado %d de MemFisica[%d] (SP Logico: %d)\n", 
                    cpu.AC, dir_fisica_pop, cpu.SP);
                    
                } else {
                    cpu.interrupcion_pendiente = 1;
                    cpu.codigo_interrupcion = 7;
                }
            break;
        }
            
        case OP_J: // 27 - Salto Incondicional (Salta siempre)
        {
            cpu.psw.pc = cpu.RB + operando;
            break;
        }

        // --- INSTRUCCIONES DE DISCO Y DMA (Fase 1) ---
            
        case OP_SDMAP: // SDMAP - Set DMA Pista
        {
            // Configura qué pista del disco queremos usar
            pthread_mutex_lock(&cpu.mutex); // Protegemos el hardware
            dma.pista_seleccionada = operando;
            pthread_mutex_unlock(&cpu.mutex);
            logger_log("      -> [SDMAP] Pista seleccionada: %d\n", operando);
            break;
        }

        case OP_SDMAC: // SDMAC - Set DMA Cilindro
        {
            // Configura qué cilindro
            pthread_mutex_lock(&cpu.mutex);
            dma.cilindro_seleccionado = operando;
            pthread_mutex_unlock(&cpu.mutex);
            logger_log("      -> [SDMAC] Cilindro seleccionado: %d\n", operando);
            break;
        }

        case OP_SDMAS: // SDMAS - Set DMA Sector
        {
            // Configura qué sector
            pthread_mutex_lock(&cpu.mutex);
            dma.sector_seleccionado = operando;
            pthread_mutex_unlock(&cpu.mutex);
            logger_log("      -> [SDMAS] Sector seleccionado: %d\n", operando);
            break;
        }

        case OP_SDMAIO: // SDMAIO - Set DMA I/O Direction
        {
            // Según tu tabla: "Establece si es I/O"
            // Usaremos el operando: 1 = Escritura (RAM->Disco), 0 = Lectura (Disco->RAM)
            pthread_mutex_lock(&cpu.mutex);
            dma.es_escritura = operando; 
            pthread_mutex_unlock(&cpu.mutex);
            logger_log("      -> [SDMAIO] Modo configurado: %s\n", 
                       dma.es_escritura ? "ESCRITURA (Grabar)" : "LECTURA (Cargar)");
            break;
        }

        case OP_SDMAM: // SDMAM - Set DMA Memory Address
        {
            // Según tu tabla: "Establece la posición de memoria a ser accedida"
            pthread_mutex_lock(&cpu.mutex);
            dma.direccion_memoria = operando;
            pthread_mutex_unlock(&cpu.mutex);
            logger_log("      -> [SDMAM] Direccion de memoria RAM objetivo: %d\n", operando);
            break;
        }

        case OP_SDMAON: // SDMAON - Encender DMA
        {
            // Esta instrucción es el "Gatillo". Arranca el hilo del DMA.
            pthread_mutex_lock(&cpu.mutex);
            dma.activo = 1; // ¡Despierta al hilo_dma en disco.c!
            pthread_mutex_unlock(&cpu.mutex);
            logger_log("      -> [SDMAON] ¡DMA ACTIVADO! Transferencia iniciada...\n");
            break;
        }

        default:
        {
            cpu.interrupcion_pendiente = 1;
            cpu.codigo_interrupcion = 5;
        }
    }
    
    return 1; // Continuar ejecutando
}

// --- HILO DEL TIMER ---
// Este código corre en paralelo a la CPU
void *hilo_timer(void *arg) {
    CPU_t *cpu = (CPU_t *)arg;

    while (cpu->ejecutando) {
        // 1. Si el timer está configurado (valor > 0)
        if (cpu->timer_periodo > 0) {
            
            // SIMULACION DE TIEMPO:
            // Para que sea visible al ojo humano, usaremos usleep.
            // Digamos que 1 ciclo simulado = 10 milisegundos.
            // Si TTI es 50, dormimos 500ms.
            usleep(cpu->timer_periodo * 10000); 

            // 2. DISPARAR INTERRUPCIÓN
            // Usamos Mutex para proteger la escritura
            pthread_mutex_lock(&cpu->mutex);
            
            if (cpu->interrupcion_pendiente == 0) { // Si no hay otra pendiente
                cpu->interrupcion_pendiente = 1;
                cpu->codigo_interrupcion = 3; // Código 3 = Reloj (según PDF)
            }
            
            pthread_mutex_unlock(&cpu->mutex);

        } else {
            // Si el timer está apagado (0), solo dormimos un poco para no quemar CPU
            usleep(100000); 
        }
    }
    return NULL;
}

void ejecutar_cpu(int modo_debug) { 
    logger_log("--- INICIANDO EJECUCION (Modo: %s) ---\n", modo_debug ? "DEBUG" : "NORMAL");
    
    // Mensaje de bienvenida del Debugger (Solo consola)
    if (modo_debug) {
        printf("\n=== MODO DEBUGGER ACTIVADO ===\n");
        printf("Comandos:\n");
        printf("  [ENTER] Siguiente Instruccion\n");
        printf("  [R]     Ver Registros Completos\n");
        printf("  [Q]     Salir de la ejecucion\n");
    }

    while (cpu.ejecutando) {
        
        // ====================================================
        // 1. FASE DE VERIFICACIÓN DE INTERRUPCIONES
        // ====================================================
        pthread_mutex_lock(&cpu.mutex); // 🔒
        
        if (cpu.interrupcion_pendiente) {
            
            // REQUISITO PDF PUNTO 3: 
            // "Cuando ocurra una interrupción debe imprimirse un mensaje tanto en el log, 
            // como en la salida estándar".
            
            // Variable auxiliar para el mensaje
            char msg[100]; 

            switch (cpu.codigo_interrupcion) {
                
                // --- ERRORES FATALES ---
                case 0: sprintf(msg, "ERROR FATAL: Codigo SVC invalido (0)"); cpu.ejecutando = 0; break;
                case 1: sprintf(msg, "ERROR FATAL: Codigo INT invalido (1)"); cpu.ejecutando = 0; break;
                case 5: sprintf(msg, "ERROR FATAL: Instruccion Ilegal (5)"); cpu.ejecutando = 0; break;
                case 6: sprintf(msg, "ERROR FATAL: Direccion Invalida (6)"); cpu.ejecutando = 0; break;
                case 7: sprintf(msg, "ERROR FATAL: Stack Underflow (7)"); cpu.ejecutando = 0; break;
                case 8: sprintf(msg, "ERROR FATAL: Stack Overflow (8)"); cpu.ejecutando = 0; break;

                // --- EVENTOS NORMALES ---
                case 2: 
                    sprintf(msg, "SYSTEM CALL (Cod 2)");
                    if (cpu.AC == 0) cpu.ejecutando = 0; 
                    break;
                case 3: sprintf(msg, "RELOJ (Cod 3)"); break;
                case 4: sprintf(msg, "FIN DE E/S (DMA) (Cod 4)"); break;
                    
                default: sprintf(msg, "DESCONOCIDO (Cod %d)", cpu.codigo_interrupcion); break;
            }
            
            // IMPRIMIMOS EN AMBOS LADOS (Cumpliendo Requisito 3)
            logger_log("\n>>> [INT] %s <<<\n", msg); // Al archivo
            printf("\n>>> [INT] %s <<<\n", msg);      // A la pantalla
            
            cpu.interrupcion_pendiente = 0; 
        }
        
        pthread_mutex_unlock(&cpu.mutex); // 🔓

        if (!cpu.ejecutando) break;

        // ====================================================
        // 2. MODO DEBUGGER (Solo Consola)
        // ====================================================
        if (modo_debug) {
            printf("\n[DEBUG] PC:%04d | Prox Instr: %08d ", 
                   cpu.psw.pc, cpu.memoria[cpu.psw.pc]);
            
            char cmd = getchar();
            if (cmd != '\n') while(getchar() != '\n'); 

            if (cmd == 'q' || cmd == 'Q') {
                printf("[DEBUG] Cancelando ejecucion...\n");
                cpu.ejecutando = 0;
                break;
            }
            else if (cmd == 'r' || cmd == 'R') {
                printf("\n   === INSPECTOR DE REGISTROS ===\n");
                printf("   AC : %08d (Int: %d)\n", cpu.AC, cpu.AC);
                printf("   MAR: %04d     | MDR: %08d\n", cpu.MAR, cpu.MDR);
                printf("   IR : %08d\n", cpu.IR);
                printf("   PC : %04d\n", cpu.psw.pc);
                printf("   SP : %04d     | RX : %04d\n", cpu.SP, cpu.RX);
                printf("   RB : %04d     | RL : %04d\n", cpu.RB, cpu.RL);
                printf("   CC : %d        | Modo : %d | Int : %d\n", 
                       cpu.psw.codigo_condicion, cpu.psw.modo_operacion, cpu.psw.interrupciones);
                printf("   ------------------------------\n");
                printf("Presione ENTER para continuar...");
                while(getchar() != '\n');
            }
        }

        // ====================================================
        // 3. FASE DE EJECUCIÓN (Silenciosa en pantalla)
        // ====================================================
        if (cpu.ejecutando) {
            if (!paso_cpu()) {
                cpu.ejecutando = 0; 
            }
            
            dump_cpu(); // Esto va al log (src/cpu.c dump_cpu usa logger_log)
            
            if (!modo_debug) usleep(50000); 
        }
    }
    
    logger_log("--- EJECUCION FINALIZADA ---\n");
    if (modo_debug) printf("\n=== FIN DEL PROGRAMA ===\nPresione ENTER para volver al menu...");
    if (modo_debug) while(getchar() != '\n');
}