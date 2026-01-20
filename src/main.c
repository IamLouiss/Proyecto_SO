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

// Hilos globales
pthread_t thread_timer_id;
pthread_t thread_dma_id;

// --- ESTRUCTURA PCB ---
typedef struct {
    int id;
    char nombre[50];    
    char archivo[50];   
    int RB;             
    int RL;             
    int pc_inicio;      
    int activo;         
} Proceso_t;

#define MAX_PROCESOS 10
#define TAMANO_PILA  200 

Proceso_t tabla_procesos[MAX_PROCESOS];
int contador_procesos = 0;

// --- FUNCIONES DE VALIDACIÓN Y ENTRADA ---
int leer_entero(const char* prompt, int def) {
    char buffer[100];
    int valor;
    
    while (1) {
        if (def != -1) printf("%s [Default: %d]: ", prompt, def);
        else printf("%s: ", prompt);

        fgets(buffer, 100, stdin);

        if (buffer[0] == '\n') {
            if (def != -1) return def;
            continue; 
        }

        char *endptr;
        valor = strtol(buffer, &endptr, 10);

        if (endptr == buffer || (*endptr != '\n' && *endptr != '\0')) {
            printf("   [!] Entrada invalida. Por favor ingrese un numero.\n");
        } else {
            return valor;
        }
    }
}

int seleccionar_archivo(char *buffer_ruta) {
    DIR *d;
    struct dirent *dir;
    d = opendir("data");
    
    if (!d) {
        printf("[ERROR] No existe la carpeta 'data/'.\n");
        return 0;
    }

    char lista_archivos[20][100]; 
    int count = 0;

    printf("\n--- ARCHIVOS DISPONIBLES EN 'data/' ---\n");
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_name[0] == '.') continue;
        strcpy(lista_archivos[count], dir->d_name);
        printf("[%d] %s\n", count + 1, dir->d_name);
        count++;
        if (count >= 20) break;
    }
    closedir(d);

    if (count == 0) {
        printf("[INFO] No hay archivos.\n");
        return 0;
    }

    int seleccion = leer_entero("Seleccione archivo (0 para cancelar)", 0);

    if (seleccion < 1 || seleccion > count) return 0;

    sprintf(buffer_ruta, "data/%s", lista_archivos[seleccion - 1]);
    return 1;
}

int verificar_colision(int inicio_nuevo, int fin_nuevo) {
    for (int i = 0; i < MAX_PROCESOS; i++) {
        if (tabla_procesos[i].activo) {
            int inicio_exist = tabla_procesos[i].RB;
            int fin_exist = tabla_procesos[i].RL;

            if (inicio_nuevo <= fin_exist && fin_nuevo >= inicio_exist) {
                printf("   [!] ERROR DE MEMORIA: El rango [%d - %d] choca con el proceso ID %d [%d - %d]\n",
                       inicio_nuevo, fin_nuevo, tabla_procesos[i].id, inicio_exist, fin_exist);
                return 1; 
            }
        }
    }
    return 0; 
}

void listar_procesos() {
    printf("\n--- PROGRAMAS CARGADOS EN MEMORIA ---\n");
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
    if (!hay) printf("   (Memoria vacia)\n");
    printf("-------------------------------------\n");
}

int main() {
    // 1. INICIALIZAR
    logger_init("logs/simulador.log");
    logger_log("--- INICIO SIMULADOR ROBUSTO ---\n");
    
    inicializar_cpu();
    inicializar_disco();

    // Solo inicializamos el Mutex una vez
    if (pthread_mutex_init(&cpu.mutex, NULL) != 0) return 1;
    
    // NOTA: Ya no creamos los hilos aquí. Se crean bajo demanda en Opción 2.

    for(int i=0; i<MAX_PROCESOS; i++) tabla_procesos[i].activo = 0;
    char ruta_archivo[150];

    // --- MENU PRINCIPAL ---
    while (1) {
        printf("\n=== GESTOR DE MEMORIA (Modo Seguro) ===\n");
        printf("1. Cargar Programa (.asm)\n");
        printf("2. Ejecutar Programa\n");
        printf("3. Salir\n");
        
        int opcion = leer_entero(">> Opcion", -1);

        if (opcion == 3) break;

        // --- CARGAR ---
        if (opcion == 1) {
            int slot = -1;
            for(int i=0; i<MAX_PROCESOS; i++) {
                if (!tabla_procesos[i].activo) { slot = i; break; }
            }
            if (slot == -1) {
                printf("[ERROR] Tabla PCB llena.\n");
                continue;
            }

            if (!seleccionar_archivo(ruta_archivo)) continue;

            char nombre_prog[50];
            int num_palabras = 0;
            if (obtener_metadatos_programa(ruta_archivo, nombre_prog, &num_palabras) == -1) {
                printf("[ERROR] No se pudo leer el archivo.\n");
                continue;
            }
            if (num_palabras == 0) num_palabras = 20; 

            int tamano_total = num_palabras + TAMANO_PILA;

            int sugerencia = 300;
            for(int i=0; i<MAX_PROCESOS; i++) {
                if(tabla_procesos[i].activo && tabla_procesos[i].RL >= sugerencia) 
                    sugerencia = tabla_procesos[i].RL + 1;
            }

            int dir_carga = 0;
            while (1) {
                dir_carga = leer_entero(">> Direccion de Carga", sugerencia);
                
                if (dir_carga < 300) {
                    printf("   [!] Error: Direccion < 300 reservada.\n");
                    continue;
                }
                
                if (dir_carga + tamano_total >= TAMANO_MEMORIA) {
                    printf("   [!] Error: El programa no cabe en RAM.\n");
                    continue;
                }

                int fin_carga = dir_carga + tamano_total;
                if (verificar_colision(dir_carga, fin_carga)) {
                    printf("   [!] Intente otra direccion.\n");
                    continue;
                }
                break;
            }

            int pc_offset = 0;
            int res = cargar_programa(ruta_archivo, dir_carga, &pc_offset, nombre_prog, &num_palabras);
            
            if (res == 0) {
                tabla_procesos[slot].id = slot + 1;
                strcpy(tabla_procesos[slot].archivo, ruta_archivo);
                strcpy(tabla_procesos[slot].nombre, nombre_prog);
                tabla_procesos[slot].RB = dir_carga;
                tabla_procesos[slot].RL = dir_carga + num_palabras + TAMANO_PILA;
                tabla_procesos[slot].pc_inicio = pc_offset;
                tabla_procesos[slot].activo = 1;

                printf("[EXITO] Programa cargado en ID %d.\n", slot+1);
            }
        }

        // --- EJECUTAR ---
        else if (opcion == 2) {
            listar_procesos();
            
            int hay = 0;
            for(int i=0; i<MAX_PROCESOS; i++) if(tabla_procesos[i].activo) hay=1;
            if(!hay) continue;

            int id_sel = leer_entero("ID a ejecutar", -1);

            int idx = -1;
            for(int i=0; i<MAX_PROCESOS; i++) {
                if (tabla_procesos[i].activo && tabla_procesos[i].id == id_sel) {
                    idx = i; break;
                }
            }

            if (idx != -1) {
                int modo = leer_entero(">> Modo (0=Normal, 1=Debug)", 0);
                if (modo != 0 && modo != 1) modo = 0;

                cpu.RB = tabla_procesos[idx].RB;
                cpu.RL = tabla_procesos[idx].RL;
                cpu.psw.pc = cpu.RB + tabla_procesos[idx].pc_inicio;
                cpu.SP = (cpu.RL - cpu.RB) - 1; 
                
                cpu.interrupcion_pendiente = 0;
                
                // === NUEVO: CICLO DE VIDA DE HILOS ===
                cpu.timer_periodo = 0; // Limpiar Timer
                dma.activo = 0;        // Limpiar DMA
                cpu.ejecutando = 1;    // Bandera ON

                // 1. Crear Hilos (Encender hardware)
                if (pthread_create(&thread_timer_id, NULL, hilo_timer, &cpu) != 0) {
                    printf("[FATAL] Error creando Timer.\n"); break;
                }
                if (pthread_create(&thread_dma_id, NULL, hilo_dma, &cpu) != 0) {
                    printf("[FATAL] Error creando DMA.\n"); break;
                }

                // 2. Ejecutar (Bloqueante hasta que termine/error)
                ejecutar_cpu(modo);

                // 3. Matar Hilos (Ya cpu.ejecutando está en 0)
                pthread_join(thread_timer_id, NULL);
                pthread_join(thread_dma_id, NULL);
                // =====================================

            } else {
                printf("   [!] Error: ID no encontrado.\n");
            }
        }
        else {
            printf("   [!] Opcion desconocida.\n");
        }
    }

    // SALIDA FINAL
    pthread_mutex_destroy(&cpu.mutex);
    logger_close();
    printf("Sistema apagado.\n");
    return 0;
}