#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h> 
#include "../include/cpu.h"
#include "../include/disco.h"
#include "../include/loader.h"
#include "../include/logger.h"

// Variables globales para manejar los hilos del hardware
pthread_t thread_timer_id;
pthread_t thread_dma_id;

// -- Estructura del PCB (Process Control Block)
// Aqui guardamos la info importante de cada programa cargado
typedef struct {
    int id;             // Identificador unico
    char nombre[50];    // El nombre interno (.NombreProg)
    char archivo[50];   // El nombre del archivo fisico
    int RB;             // Donde empieza en la RAM
    int RL;             // Donde termina
    int pc_inicio;      // En que instruccion arranca (_start)
    int activo;         // 1 = Ocupado, 0 = Libre
} Proceso_t;

#define MAX_PROCESOS 10   // Maximo de programas en memoria a la vez
#define TAMANO_PILA  100  // Espacio reservado para el Stack de cada proceso

// Tabla donde llevamos el control de la memoria
Proceso_t tabla_procesos[MAX_PROCESOS];

// -- Funciones auxiliares

// Lee un numero de la consola
int leer_entero(const char* prompt, int def) {
    char buffer[100];
    int valor;
    
    while (1) {
        // Mostramos el mensaje bonito con el valor por defecto
        if (def != -1) printf("%s [Default: %d]: ", prompt, def);
        else printf("%s: ", prompt);

        fgets(buffer, 100, stdin);

        // Si solo dio Enter, usamos la sugerencia
        if (buffer[0] == '\n') {
            if (def != -1) return def;
            continue; // Si no hay default, obligamos a escribir algo
        }

        // Intentamos convertir a número
        char *endptr;
        valor = strtol(buffer, &endptr, 10);

        // Validamos que no haya escrito basura (ej: "300abc")
        if (endptr == buffer || (*endptr != '\n' && *endptr != '\0')) {
            printf("   [!] Eso no es un numero valido. Intenta de nuevo.\n");
        } else {
            return valor;
        }
    }
}

// Muestra los archivos de la carpeta 'data' para no tener que escribirlos a mano
int seleccionar_archivo(char *buffer_ruta) {
    DIR *d;
    struct dirent *dir;
    d = opendir("data");
    
    if (!d) {
        printf("Error: No existe la carpeta 'data/'.\n");
        return 0;
    }

    char lista_archivos[20][100]; 
    int count = 0;

    printf("\nArchivos disponibles:\n");
    while ((dir = readdir(d)) != NULL) {
        // Saltamos los archivos ocultos (los que empiezan con punto)
        if (dir->d_name[0] == '.') continue;
        
        strcpy(lista_archivos[count], dir->d_name);
        printf("[%d] %s\n", count + 1, dir->d_name);
        count++;
        if (count >= 20) break;
    }
    closedir(d);

    if (count == 0) {
        printf("Info: La carpeta esta vacia.\n");
        return 0;
    }

    int seleccion = leer_entero("Elige el numero del archivo (0 para salir)", 0);

    if (seleccion < 1 || seleccion > count) return 0;

    // Guardamos la ruta completa para que el loader la encuentre
    sprintf(buffer_ruta, "data/%s", lista_archivos[seleccion - 1]);
    return 1;
}

// Revisa que el nuevo programa no se monte encima de uno viejo
// Retorna 1 si hay choque (Colisión), 0 si está libre
int verificar_colision(int inicio_nuevo, int fin_nuevo) {
    for (int i = 0; i < MAX_PROCESOS; i++) {
        if (tabla_procesos[i].activo) {
            int inicio_exist = tabla_procesos[i].RB;
            int fin_exist = tabla_procesos[i].RL;

            // Verificacion de solapamiento de memoria de archivos
            if (inicio_nuevo <= fin_exist && fin_nuevo >= inicio_exist) {
                printf("   [!] ERROR: Ese espacio esta ocupado por el ID %d [%d - %d]\n",
                       tabla_procesos[i].id, inicio_exist, fin_exist);
                return 1; // Se solapan
            }
        }
    }
    return 0; // No se solapan
}

// Imprime la tabla PCB para ver qué tenemos cargado
void listar_procesos() {
    printf("\nProgramas cargados:\n");
    int hay = 0;
    for(int i=0; i<MAX_PROCESOS; i++) {
        if(tabla_procesos[i].activo) {
            printf("[%d] %-10s | RB:%04d - RL:%04d | StartOffset:%d\n",
                tabla_procesos[i].id,
                tabla_procesos[i].nombre,
                tabla_procesos[i].RB,
                tabla_procesos[i].RL,
                tabla_procesos[i].pc_inicio);
            hay = 1;
        }
    }
    if (!hay) printf("   Memoria vacia\n");
    printf("----------------------------\n");
}

int main() {
    // 1. Arrancamos el sistema
    logger_init("logs/simulador.log");
    logger_log("--- INICIO DEL SIMULADOR ---\n");
    
    inicializar_cpu();
    inicializar_disco();

    // Creamos el candado (Mutex)
    if (pthread_mutex_init(&cpu.mutex, NULL) != 0) return 1;

    // Inicializamos la variable de condicion
    if (pthread_cond_init(&cpu.cond_dma, NULL) != 0) return 1;
    
    // Limpiamos la tabla de procesos
    for(int i=0; i<MAX_PROCESOS; i++) tabla_procesos[i].activo = 0;
    char ruta_archivo[150];

    // -- Ciclo del menu principal
    while (1) {
        printf("\n=== GESTOR DE MEMORIA ===\n");
        printf("1. Cargar Programa\n");
        printf("2. Ejecutar Programa Cargado\n");
        printf("3. Apagar Sistema\n");
        
        int opcion = leer_entero(">>Opcion", -1);

        if (opcion == 3) break;

        // -- Opcion 1: Cargar programa en memoria
        if (opcion == 1) {
            // Buscamos un hueco libre en la tabla PCB
            int slot = -1;
            for(int i=0; i<MAX_PROCESOS; i++) {
                if (!tabla_procesos[i].activo) { slot = i; break; }
            }
            if (slot == -1) {
                printf("Error: Ya tienes muchos programas cargados (%d).\n", MAX_PROCESOS);
                continue;
            }

            if (!seleccionar_archivo(ruta_archivo)) continue;

            // Leemos el encabezado sin cargar todavía para saber cuanto mide
            char nombre_prog[50];
            int num_palabras = 0;
            if (obtener_datos_programa(ruta_archivo, nombre_prog, &num_palabras) == -1) {
                printf("Error: No pude leer el archivo.\n");
                continue;
            }

            // Calculamos el espacio total: Codigo + Pila
            int tamano_total = num_palabras + TAMANO_PILA;

            // Buscamos donde termina el ultimo programa para sugerir esa direccion
            int sugerencia = 300;
            for(int i=0; i<MAX_PROCESOS; i++) {
                if(tabla_procesos[i].activo && tabla_procesos[i].RL >= sugerencia) 
                    sugerencia = tabla_procesos[i].RL + 1;
            }

            // Validamos que la direccion elegida sea segura
            int dir_carga = 0;
            while (1) {
                dir_carga = leer_entero(">> Direccion de Inicio (RB)", sugerencia);
                
                // Zona prohibida (Sistema Operativo)
                if (dir_carga < 300) {
                    printf("   [!] Cuidado: Las primeras 300 posiciones son del Kernel.\n");
                    continue;
                }
                
                // Zona prohibida (Fuera de la RAM fisica)
                if (dir_carga + tamano_total >= TAMANO_MEMORIA) {
                    printf("   [!] Error: El programa no cabe en la RAM.\n");
                    continue;
                }

                // Zona ocupada (Otro proceso)
                int fin_carga = dir_carga + tamano_total;
                if (verificar_colision(dir_carga, fin_carga)) {
                    printf("   [!] Busca otra direccion.\n");
                    continue;
                }
                break;
            }

            // Si pasamos las validaciones, cargamos de verdad
            int pc_offset = 0;
            int res = cargar_programa(ruta_archivo, dir_carga, &pc_offset, nombre_prog, &num_palabras);
            
            if (res != -1) {
                // Guardamos los datos en el PCB
                tabla_procesos[slot].id = slot + 1;
                strcpy(tabla_procesos[slot].archivo, ruta_archivo);
                strcpy(tabla_procesos[slot].nombre, nombre_prog);
                tabla_procesos[slot].RB = dir_carga;
                // El limite es: Inicio + Codigo + Pila
                tabla_procesos[slot].RL = dir_carga + num_palabras + TAMANO_PILA;
                tabla_procesos[slot].pc_inicio = pc_offset;
                tabla_procesos[slot].activo = 1;

                printf("[EXITO] Programa cargado con ID %d.\n", slot+1);
            }
        }

        // -- Opcion 2: Ejecutar
        else if (opcion == 2) {
            listar_procesos();
            
            // Verificamos si hay algo que correr
            int hay = 0;
            for(int i=0; i<MAX_PROCESOS; i++) if(tabla_procesos[i].activo) hay=1;
            if(!hay) continue;

            int id_sel = leer_entero("Escribe el ID del programa", -1);

            // Buscamos ese ID en la tabla
            int idx = -1;
            for(int i=0; i<MAX_PROCESOS; i++) {
                if (tabla_procesos[i].activo && tabla_procesos[i].id == id_sel) {
                    idx = i; break;
                }
            }

            if (idx != -1) {
                int modo = leer_entero(">> Modo (0=Rapido, 1=Paso a Paso)", 0);
                if (modo != 0 && modo != 1) modo = 0;

                // -- Cambio de Contexto
                // Aqui configuramos la CPU para el proceso elegido
                
                cpu.RB = tabla_procesos[idx].RB; // Base del proceso
                cpu.RL = tabla_procesos[idx].RL; // Techo del proceso
                cpu.RX = cpu.RL - TAMANO_PILA;
                cpu.psw.pc = cpu.RB + tabla_procesos[idx].pc_inicio; // PC arranca donde diga el _start (relativo al RB)
                cpu.SP = (cpu.RL - cpu.RB) - 1; 
                cpu.interrupcion_pendiente = 0;
                
                // -- Encendemos los hilos y cpu
                cpu.timer_periodo = 0; 
                dma.activo = 0;        
                cpu.ejecutando = 1;

                // Creamos los hilos
                if (pthread_create(&thread_timer_id, NULL, hilo_timer, &cpu) != 0) {
                    printf("[FATAL] Fallo al crear el Timer.\n"); break;
                }
                if (pthread_create(&thread_dma_id, NULL, hilo_dma, &cpu) != 0) {
                    printf("[FATAL] Fallo al crear el DMA.\n"); break;
                }

                // Corremos la CPU con el programa seleccionado
                ejecutar_cpu(modo);

                // -- Proceso para apagar el hardware
                // Despertamos al DMA por si se quedo dormido esperando una orden
                pthread_mutex_lock(&cpu.mutex);
                pthread_cond_signal(&cpu.cond_dma); 
                pthread_mutex_unlock(&cpu.mutex);

                // Esperamos a que terminen los hilos
                pthread_join(thread_timer_id, NULL);
                pthread_join(thread_dma_id, NULL);

            } else {
                printf("   [!] Ese ID no existe.\n");
            }
        }
        else {
            printf("   [!] Opcion incorrecta.\n");
        }
    }

    // Salida limpia
    pthread_mutex_destroy(&cpu.mutex);
    pthread_cond_destroy(&cpu.cond_dma);
    logger_close();
    printf("Sistema apagado correctamente.\n");
    return 0;
}