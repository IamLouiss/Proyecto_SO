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

    // Configuracion inicial del PSW
    cpu.psw.modo_operacion = 1;      // Arranca en modo kernel (1)
    cpu.psw.interrupciones = 0;      // Deshabilitadas al inicio
    cpu.psw.codigo_condicion = 0;
    
    
    // El PC debe apuntar a donde arrancara el programa
    // Por ahora lo ponemos en el inicio de usuario
    cpu.psw.pc = INICIO_USUARIO;     
    
    // Inicializar registros de proteccion (Todo el espacio del usuario)
    cpu.RB = INICIO_USUARIO;
    cpu.RL = TAMANO_MEMORIA - 1;
    cpu.SP = (cpu.RL - cpu.RB) - 1;
    
    // Bandera para el bucle principal
    cpu.ejecutando = 1;

    logger_log("Info: CPU inicializada. Modo Kernel. Memoria limpia (0-1999).\n");
}

void dump_cpu() {
    // Esta funcion nos servira para ver el estado de los registros, pila, etc.
    logger_log("\n=== ESTADO CPU ===\n");
    logger_log("PC: %04d | IR: %08d | AC: %08d\n", cpu.psw.pc, cpu.IR, cpu.AC);
    logger_log("MAR: %04d | MDR: %08d\n", cpu.MAR, cpu.MDR);
    logger_log("PSW: CC=%d Modo=%d Int=%d\n", 
           cpu.psw.codigo_condicion, cpu.psw.modo_operacion, cpu.psw.interrupciones);
    logger_log("Stack: SP=%d RX=%d\n", cpu.SP, cpu.RX);
    logger_log("RB=%d | RL=%d\n", cpu.RB, cpu.RL);
    logger_log("==================\n");
}

// Valida si una direccion fisica es legal para el proceso actual
// Retorna 1 si es valida, 0 si es ilegal (y dispara interrupcion)
int validar_direccion(int dir_fisica) {
    // Si estamos en modo kernel (1) todo esta permitido
    if (cpu.psw.modo_operacion == 1) {
        return 1;
    }

    // En modo usuario (0) verificamos los limites
    if (dir_fisica < cpu.RB || dir_fisica >= cpu.RL) {
        logger_log("[INT] Violacion de segmento: Dir %d fuera de rango (%d-%d)\n", 
               dir_fisica, cpu.RB, cpu.RL);
        return 0;
    }
    return 1;
}

// Obtiene el valor real del operando segun el modo
// Retorna el valor listo para usar en sumas, cargas, etc.
int obtener_valor_operando(int modo, int operando) {
    int valor = 0;
    int direccion_final = 0;

    switch (modo) {

        case DIR_DIRECTO: // Modo 0
        {
            // Calculamos direccion fisica
            direccion_final = operando + cpu.RB; 
            if (validar_direccion(direccion_final)) {
                valor = cpu.memoria[direccion_final];
            }
            break;
        }

        case DIR_INMEDIATO: // Modo 1
        {
            valor = operando;
            break;
        }
            
        case DIR_INDEXADO: // Modo 2
        {
            // El PDF especifica que el indice es a partir del acumulador
            direccion_final = operando + cpu.AC + cpu.RB;
            if (validar_direccion(direccion_final)) {
                valor = cpu.memoria[direccion_final];
            }
            break;
        }
    }
    return valor;
}

int paso_cpu() {
    int instruccion, opcode, modo, operando;

    // -- FETCH (Busqueda)
    // 1. MAR <- PC
    cpu.MAR = cpu.psw.pc;
    
    // 2. Validar acceso a memoria
    if (!validar_direccion(cpu.MAR)) return 0;

    // 3. MDR <- Memoria[MAR]
    cpu.MDR = cpu.memoria[cpu.MAR];

    // 4. IR <- MDR
    cpu.IR = cpu.MDR;

    // 5. PC++ (Apunta a la siguiente instruccion)
    cpu.psw.pc++;

    // -- DECODE (Decodificacion)
    // Formato: OPCODE (2 digitos) | MODO (1 digito) | OPERANDO (5 digitos)
    instruccion = cpu.IR;
    
    // Usamos div y mod para separar la instruccion en trozos y obtener cada parte
    opcode = (instruccion / 1000000);     // Los primeros 2 digitos
    modo = (instruccion / 100000) % 10;   // El 3er digito
    operando = (instruccion % 100000);    // Los ultimos 5 digitos
    
    // Escribimos en el log los registros para llevar control de lo que se realizo
    logger_log("Exec: PC:%04d | IR:%08d -> OP:%02d M:%d VAL:%05d\n", 
        cpu.MAR, instruccion, opcode, modo, operando);
        
    // -- EXECUTE (Ejecucion)
    switch (opcode) {

        case OP_SUM: // 00 - Suma
        {
            // Logica: AC = AC + valor
            int val = obtener_valor_operando(modo, operando);
            long long resultado_temp = (long long)cpu.AC + val; // Usamos long long para ver si se pasa

            // Verificamos si hay algun desbordamiento (overflow/underflow)
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
                // Actualizamos CC, lo cual permite hacer saltos condicionales basados en el resultado
                if (cpu.AC == 0) cpu.psw.codigo_condicion = 0;     // Para JMP Equal
                else if (cpu.AC < 0) cpu.psw.codigo_condicion = 1; // Para JMP Less Than
                else cpu.psw.codigo_condicion = 2;                 // Para JMP Greater Than
            }
            break;
        }    

        case OP_RES: // 01 - Restar
        {
            // Logica: AC = AC - valor
            int val = obtener_valor_operando(modo, operando);
            long long resultado_temp = (long long)cpu.AC - val;

            // Utiliza la misma logica que OP_SUM para el desbordamiento y CC
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
            // Logica: AC = AC * valor
            int val = obtener_valor_operando(modo, operando);
            long long resultado_temp = (long long)cpu.AC * val;

            // Utiliza la misma logica que OP_SUM para el desbordamiento y CC
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
            // Logica: AC = AC / Valor
            // Utiliza la misma logica que OP_SUM para el CC
            int val = obtener_valor_operando(modo, operando);
            if (val != 0) {
                cpu.AC /= val;
                if (cpu.AC == 0) cpu.psw.codigo_condicion = 0;
                else if (cpu.AC < 0) cpu.psw.codigo_condicion = 1;
                else cpu.psw.codigo_condicion = 2;
            } else {
                cpu.psw.codigo_condicion = 3; // Usamos el codigo 3 para error matematico
                cpu.interrupcion_pendiente = 1;
                cpu.codigo_interrupcion = 8;  // Utilizamos el codigo de interrupcion 8 para el caso de division por cero
            }
            break;
        }
            
        case OP_LOAD: // 04 - Cargar en AC
        {
            // Logica: AC = Valor
            cpu.AC = obtener_valor_operando(modo, operando);
            break;
        }
            
        case OP_STR: // 05 - Guardar AC en memoria
        {
            // Logica: Memoria[Operando] = AC
            int dir_destino = -1;
            if (modo == DIR_DIRECTO) {
                dir_destino = operando + cpu.RB;
            } else if (modo == DIR_INDEXADO) {
                dir_destino = operando + cpu.RB + cpu.AC;
            }

            // Solo escribimos si la dirección es valida
            if (dir_destino != -1 && validar_direccion(dir_destino)) {
                cpu.memoria[dir_destino] = cpu.AC;
                logger_log("      -> Guardado %d en Mem[%d]\n", cpu.AC, dir_destino);
            } else {
                cpu.interrupcion_pendiente = 1; // En caso de error tendriamos el codigo de interrupcion 6
                cpu.codigo_interrupcion = 6;    // que es direccionamiento invalido
            }
            break;
        }

        case OP_LOADRX: // 06 - Cargar en RX
        {
            // Logica: RX = Valor
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
                dir_destino = operando + cpu.RB + cpu.AC;
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
        
        case OP_SVC: // 13 - System Call (Llamada al Sistema)
        {
            // Se usa para solicitar servicios al kernel
            // En esta Fase 1, si AC=0 asumimos que se pide terminar la simulacion
            cpu.interrupcion_pendiente = 1;
            cpu.codigo_interrupcion = 2; // Codigo de interrupcion de llamada al sistema

            logger_log("      -> SVC: Llamada al sistema detectada. Codigo en AC: %d\n", cpu.AC);
            if (cpu.AC == 0) {
                logger_log("      -> Info: SVC 0: Solicitud de fin de programa.\n");
                cpu.ejecutando = 0; // Detiene el bucle principal
            }
            break;
        }
        
        case OP_RETRN: // 14 - Return
        {
            // Recupera el valor del PC que estaba guardado en el tope de la pila
            // Esto permite volver al lugar donde se llamo a la funcion
            int tope = (cpu.RL - cpu.RB) - 1;
            if (cpu.SP < tope) {
                cpu.SP++; // Pasamos de la posicion vacia a la llena
                cpu.psw.pc = cpu.memoria[cpu.SP + cpu.RB]; // Leemos la direccion de retorno
                logger_log("      -> RETRN: Retornando a la direccion %d (Stack[%d])\n", cpu.psw.pc, cpu.SP);
            } else {
                cpu.interrupcion_pendiente = 1;
                cpu.codigo_interrupcion = 7; // Stack Underflow
            }
            break;
        }
        
        case OP_HAB: // 15 - Habilitar Interrupciones
        {
            cpu.psw.interrupciones = 1;
            logger_log("      -> HAB: Interrupciones habilitadas.\n");
            break;
        }
        
        case OP_DHAB: // 16 - Deshabilitar Interrupciones
        {
            cpu.psw.interrupciones = 0;
            logger_log("      -> DHAB: Interrupciones deshabilitadas.\n");
            break;
        }
        
        case OP_TTI: // 17 - Configurar Timer
        {
            // Ahora si guardamos el valor para que el hilo lo lea
            pthread_mutex_lock(&cpu.mutex); // Protegemos el cambio
            cpu.timer_periodo = operando;
            pthread_mutex_unlock(&cpu.mutex);
            
            logger_log("      -> TTI: Timer configurado a %d ciclos.\n", 
                operando);
            break;
        }
            
        case OP_CHMOD: // 18 - Cambiar modo (Kernel - Usuario)
        {
            if (cpu.psw.modo_operacion == 0) {  // Si esta en modo usuario no tiene los permisos para cambiar de modo
                cpu.interrupcion_pendiente = 1;
                cpu.codigo_interrupcion = 5; // Codigo de interrupcion de instruccion invalida
                logger_log("      -> Error: Violacion de Privilegios\n");
            } else if (operando == 0 || operando == 1) { // En modo kernel si puedes cambiar de modo por lo tanto valida el operando
                cpu.psw.modo_operacion = operando;
                logger_log("      -> CHMOD: Modo: %d\n", operando);
            } else {
                cpu.interrupcion_pendiente = 1;
                cpu.codigo_interrupcion = 5; // Argumento invalido
            }
            break;
        }
            
        case OP_LOADRB: // 19 - Cargar Registro Base
        {
            cpu.AC = cpu.RB; 
            logger_log("      -> LOADRB: AC cargado con RB (%d)\n", cpu.RB);
            break;
        }
        
        case OP_STRRB:  // 20 - Guardar en Registro Base
        {
            cpu.RB = cpu.AC; 
            logger_log("      -> STRRB: RB actualizado con AC (%d)\n", cpu.RB);
            break;
        }
        
        case OP_LOADRL: // 21 - Cargar Registro Limite
        {
            cpu.AC = cpu.RL; 
            logger_log("      -> LOADRL: AC cargado con RL (%d)\n", cpu.RL);
            break;
        }
        
        case OP_STRRL:  // 22 - Guardar en Registro Limite
        {
            cpu.RL = cpu.AC; 
            logger_log("      -> STRRL: RL actualizado con AC (%d)\n", cpu.RL);
            break;
        }
        
        case OP_LOADSP: // 23 - Cargar Stack Pointer a AC
        {
            cpu.AC = cpu.SP;
            logger_log("      -> LOADSP: AC cargado con SP (%d)\n", cpu.AC);
            break;
        }
        
        case OP_STRSP:  // 24 - Actualizar Stack Pointer desde AC
        {
            cpu.SP = cpu.AC;
            logger_log("      -> STRSP: SP actualizado con AC (%d)\n", cpu.SP);
            break;
        }
        
        case OP_PSH: // 25 - PUSH
        {
            // 1. Calculamos la direccion fisica real
            int dir_fisica = cpu.RB + cpu.SP;
            
            // 2. Verificamos seguridad
            if (dir_fisica >= cpu.RX && dir_fisica < cpu.RL) {
                
                cpu.memoria[dir_fisica] = cpu.AC; 
                logger_log("      -> PSH: Valor %d apilado en MemFisica[%d] (SP Logico: %d)\n", 
                cpu.AC, dir_fisica, cpu.SP);
                cpu.SP--;
            } else {
                cpu.interrupcion_pendiente = 1;
                cpu.codigo_interrupcion = 8; // Stack Overflow
            }
            break;
        }
        
        case OP_POP: // 26 - POP
        {
            int tope = (cpu.RL - cpu.RB) - 1;
            if (cpu.SP < tope) { // Validamos si hay datos en la pila
                
                // Incrementamos el puntero
                cpu.SP++; 
                
                // Calculamos la direccion fisica
                int dir_fisica_pop = cpu.RB + cpu.SP;
                
                // Leemos el dato
                cpu.AC = cpu.memoria[dir_fisica_pop];
                
                logger_log("      -> POP: Recuperado %d de MemFisica[%d] (SP Logico: %d)\n", 
                    cpu.AC, dir_fisica_pop, cpu.SP);
                    
                } else {
                    cpu.interrupcion_pendiente = 1;
                    cpu.codigo_interrupcion = 7; // Stack Underflow
                }
            break;
        }
            
        case OP_J: // 27 - Salto Incondicional
        {
            cpu.psw.pc = cpu.RB + operando;
            break;
        }
            
        case OP_SDMAP: // 28 - Set DMA Pista
        {
            // Configura que pista del disco queremos usar
            pthread_mutex_lock(&cpu.mutex); // Protegemos el hardware
            dma.pista_seleccionada = operando;
            pthread_mutex_unlock(&cpu.mutex);
            logger_log("      -> SDMAP: Pista seleccionada: %d\n", operando);
            break;
        }

        case OP_SDMAC: // 29 - Set DMA Cilindro
        {
            // Configura que cilindro del disco queremos usar
            pthread_mutex_lock(&cpu.mutex);
            dma.cilindro_seleccionado = operando;
            pthread_mutex_unlock(&cpu.mutex);
            logger_log("      -> SDMAC: Cilindro seleccionado: %d\n", operando);
            break;
        }

        case OP_SDMAS: // 30 - Set DMA Sector
        {
            // Configura que sector del disco queremos usar
            pthread_mutex_lock(&cpu.mutex);
            dma.sector_seleccionado = operando;
            pthread_mutex_unlock(&cpu.mutex);
            logger_log("      -> SDMAS: Sector seleccionado: %d\n", operando);
            break;
        }

        case OP_SDMAIO: // 31 - Set DMA I/O
        {
            // Establece si es escritura o lectura
            // Usaremos el operando: 1 = Escritura (RAM->Disco), 0 = Lectura (Disco->RAM)
            pthread_mutex_lock(&cpu.mutex);
            dma.es_escritura = operando; 
            pthread_mutex_unlock(&cpu.mutex);
            logger_log("      -> SDMAIO: Modo configurado: %s\n", 
                       dma.es_escritura ? "escritura" : "lectura");
            break;
        }

        case OP_SDMAM: // 32 - Set DMA Memory Address
        {
            // Establece la posición de memoria a ser accedida
            int dir_fisica = operando + cpu.RB;

            pthread_mutex_lock(&cpu.mutex);
            dma.direccion_memoria = dir_fisica;
            pthread_mutex_unlock(&cpu.mutex);
            logger_log("      -> SDMAM: Direccion de memoria RAM objetivo: %d\n", operando);
            break;
        }

        case OP_SDMAON: // 33 - Encender DMA
        {
            pthread_mutex_lock(&cpu.mutex);
            dma.activo = 1; 
            
            // Despertamos al hilo del disco
            pthread_cond_signal(&cpu.cond_dma); 
            
            pthread_mutex_unlock(&cpu.mutex);
            logger_log("      -> SDMAON: DMA activado\n");
            break;
        }

        default:
        {
            cpu.interrupcion_pendiente = 1;
            cpu.codigo_interrupcion = 5; // Instruccion invalida
        }
    }
    
    return 1; // Continuar ejecutando
}

// -- Hilo del Timer
// Este codigo corre en paralelo a la CPU
void *hilo_timer(void *arg) {
    CPU_t *cpu = (CPU_t *)arg;

    while (cpu->ejecutando) {
        // 1. Verificamos si el Timer esta configurado
        if (cpu->timer_periodo > 0) {
            
            // Agregamos cierto tiempo para que no se sature el hilo
            usleep(cpu->timer_periodo * 1000); 

            // 2. Disparar interrupcion cuando acabe el timer
            // Usamos Mutex para proteger la escritura
            pthread_mutex_lock(&cpu->mutex);
            
            if (cpu->interrupcion_pendiente == 0) { // Si no hay otra pendiente
                cpu->interrupcion_pendiente = 1;
                cpu->codigo_interrupcion = 3; // Codigo 3 = Reloj
            }
            
            pthread_mutex_unlock(&cpu->mutex);

        } else {
            // Si el timer está apagado (0), dormimos 100ms para no quemar CPU
            usleep(100000); 
        }
    }
    return NULL;
}

void ejecutar_cpu(int modo_debug) { 
    logger_log("--- Iniciando ejecucion en modo: %s ---\n", modo_debug ? "debug" : "normal");
    
    // Mensaje de bienvenida del Debugger en consola
    if (modo_debug) {
        printf("\n=== MODO DEBUGGER ACTIVADO ===\n");
        printf("Comandos:\n");
        printf("  [ENTER] Siguiente Instruccion\n");
        printf("  [R]     Ver Registros Completos\n");
        printf("  [Q]     Salir de la ejecucion\n");
    }

    while (cpu.ejecutando) {
        
        // Verificacion de interrupciones
        pthread_mutex_lock(&cpu.mutex); // Colocamos el candado
        
        if (cpu.interrupcion_pendiente) {
            
            // Cuando ocurre una interrupcion debe imprimirse un mensaje tanto en el log, 
            // como en la salida estandar
            
            // Variable auxiliar para el mensaje
            char msg[100]; 

            switch (cpu.codigo_interrupcion) {
                
                // -- Interrupciones que finalizan el programa
                case 0: sprintf(msg, "ERROR FATAL: Codigo de llamada al sistema invalido (0)"); cpu.ejecutando = 0; break;
                case 1: sprintf(msg, "ERROR FATAL: Codigo de interrupcion invalido (1)"); cpu.ejecutando = 0; break;
                case 5: sprintf(msg, "ERROR FATAL: Instruccion ilegal (5)"); cpu.ejecutando = 0; break;
                case 6: sprintf(msg, "ERROR FATAL: Direccion invalida (6)"); cpu.ejecutando = 0; break;
                case 7: sprintf(msg, "ERROR FATAL: Stack underflow (7)"); cpu.ejecutando = 0; break;
                case 8: sprintf(msg, "ERROR FATAL: Stack overflow (8)"); cpu.ejecutando = 0; break;

                // -- Interrupciones que no afectan la ejecucion del programa
                case 2: 
                    sprintf(msg, "Llamada al sistema (2)");
                    if (cpu.AC == 0) cpu.ejecutando = 0; 
                    break;
                case 3: sprintf(msg, "Reloj (3)"); break;
                case 4: sprintf(msg, "Fin de E/S (DMA) (4)"); break;
                    
                default: sprintf(msg, "Desconocido (%d)", cpu.codigo_interrupcion); break;
            }
            
            // Imprimimos tanto en el logger como en consola
            logger_log("\n>>> Int: %s <<<\n", msg);
            printf("\n>>> Int: %s <<<\n", msg);
            
            cpu.interrupcion_pendiente = 0; 
        }
        
        pthread_mutex_unlock(&cpu.mutex);

        if (!cpu.ejecutando) break;

        // -- Modo debugger
        if (modo_debug) {
            printf("\nDebug: PC:%04d | Prox Instr: %08d ", 
                   cpu.psw.pc, cpu.memoria[cpu.psw.pc]);
            
            char cmd = getchar(); // Leemos un caracter de la entrada estandar
            if (cmd != '\n') while(getchar() != '\n'); // Enter para avanzar

            if (cmd == 'q' || cmd == 'Q') {
                printf("Debug: Ejecucion cancelada\n");
                cpu.ejecutando = 0;
                break;
            }
            else if (cmd == 'r' || cmd == 'R') {
                printf("\n   === REGISTROS ===\n");
                printf("   AC : %08d (Int: %d)\n", cpu.AC, cpu.AC);
                printf("   MAR: %04d     | MDR: %08d\n", cpu.MAR, cpu.MDR);
                printf("   IR : %08d\n", cpu.IR);
                printf("   PC : %04d\n", cpu.psw.pc);
                printf("   SP : %04d     | RX : %04d\n", cpu.SP, cpu.RX);
                printf("   RB : %04d     | RL : %04d\n", cpu.RB, cpu.RL);
                printf("   CC : %d       | Modo : %d | Int : %d\n", 
                       cpu.psw.codigo_condicion, cpu.psw.modo_operacion, cpu.psw.interrupciones);
                printf("   ------------------------------\n");
                printf("Presione ENTER para continuar");
                while(getchar() != '\n');
            }
        }

        // -- Fase de ejecucion
        if (cpu.ejecutando) {
            if (!paso_cpu()) {
                cpu.ejecutando = 0; 
            }
            
            dump_cpu(); // Esto va al log para que se vea el estado al ejecutar cada instruccion
            
            // Para que el cpu se ejecute un poco mas lento y le de tiempo al timer
            // de mostrar interrupciones de reloj 
            if (!modo_debug) usleep(50000);
        }
    }
    
    logger_log("--- Ejecucion finalizada ---\n");
    if (modo_debug) printf("\n=== Fin del programa ===\nPresione ENTER para volver al menu");
    if (modo_debug) while(getchar() != '\n');
}