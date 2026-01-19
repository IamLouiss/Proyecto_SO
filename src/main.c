#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
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
    char nombre[50];    // Nombre sacado de .NombreProg
    char archivo[50];   // Nombre del archivo .asm
    int RB;             // Inicio
    int RL;             // Límite (Instrucciones + Pila)
    int pc_inicio;      // Offset _start
    int activo;         // 1=Ocupado
} Proceso_t;

#define MAX_PROCESOS 10
#define TAMANO_PILA  200 // Definido por tu amigo

Proceso_t tabla_procesos[MAX_PROCESOS];
int contador_procesos = 0;

void limpiar_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
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

    printf("[0] Cancelar\n");
    printf("Seleccione archivo: ");
    
    int seleccion = 0;
    if (scanf("%d", &seleccion) != 1) { 
        limpiar_buffer(); return 0; 
    }
    limpiar_buffer();

    if (seleccion < 1 || seleccion > count) return 0;

    sprintf(buffer_ruta, "data/%s", lista_archivos[seleccion - 1]);
    return 1;
}

void listar_procesos() {
    printf("\n--- PROGRAMAS CARGADOS EN MEMORIA ---\n");
    int hay = 0;
    for(int i=0; i<MAX_PROCESOS; i++) {
        if(tabla_procesos[i].activo) {
            printf("[%d] Nombre: %-10s | Archivo: %-15s | RB:%d RL:%d | Start:%d\n",
                tabla_procesos[i].id,
                tabla_procesos[i].nombre,
                tabla_procesos[i].archivo,
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
    logger_log("--- INICIO SIMULADOR ---\n");
    
    inicializar_cpu();
    inicializar_disco();

    if (pthread_mutex_init(&cpu.mutex, NULL) != 0) return 1;
    cpu.timer_periodo = 0;
    cpu.interrupcion_pendiente = 0;

    if (pthread_create(&thread_timer_id, NULL, hilo_timer, &cpu) != 0) return 1;
    if (pthread_create(&thread_dma_id, NULL, hilo_dma, &cpu) != 0) return 1;

    for(int i=0; i<MAX_PROCESOS; i++) tabla_procesos[i].activo = 0;
    int opcion = 0;
    char ruta_archivo[150];

    // --- MENU PRINCIPAL ---
    while (1) {
        printf("\n=== GESTOR DE MEMORIA ===\n");
        printf("1. Cargar Programa (.asm)\n");
        printf("2. Ejecutar Programa\n");
        printf("3. Salir\n");
        printf(">> ");
        
        if (scanf("%d", &opcion) != 1) {
            limpiar_buffer(); continue;
        }
        limpiar_buffer();

        if (opcion == 3) break;

        // --- CARGAR ---
        if (opcion == 1) {
            // 1. Buscar slot libre
            int slot = -1;
            for(int i=0; i<MAX_PROCESOS; i++) {
                if (!tabla_procesos[i].activo) { slot = i; break; }
            }
            if (slot == -1) {
                printf("[ERROR] No hay slots libres en PCB.\n");
                continue;
            }

            // 2. Elegir archivo
            if (!seleccionar_archivo(ruta_archivo)) continue;

            // 3. Calcular dirección sugerida (Siguiente espacio libre)
            int sugerencia = 300;
            for(int i=0; i<MAX_PROCESOS; i++) {
                if(tabla_procesos[i].activo && tabla_procesos[i].RL >= sugerencia) 
                    sugerencia = tabla_procesos[i].RL + 1; // Pegadito al anterior
            }

            printf(">> Direccion de Carga (Sugerido: %d): ", sugerencia);
            int dir_carga = 300;
            char entrada[20];
            fgets(entrada, 20, stdin);
            if (entrada[0] != '\n') dir_carga = atoi(entrada);
            if (dir_carga < 300) dir_carga = 300;

            // 4. Llamar al Loader
            int pc_offset = 0;
            char nombre_prog[50];
            int num_palabras = 0;

            if (cargar_programa(ruta_archivo, dir_carga, &pc_offset, nombre_prog, &num_palabras) == 0) {
                
                // GUARDAR EN PCB
                tabla_procesos[slot].id = slot + 1;
                strcpy(tabla_procesos[slot].archivo, ruta_archivo);
                strcpy(tabla_procesos[slot].nombre, nombre_prog); // El nombre que venía en el .asm
                tabla_procesos[slot].RB = dir_carga;
                
                // --- CALCULO CLAVE DEL RL ---
                // RL = Inicio + (Instrucciones) + (Pila de 200)
                tabla_procesos[slot].RL = dir_carga + num_palabras + TAMANO_PILA;
                
                tabla_procesos[slot].pc_inicio = pc_offset;
                tabla_procesos[slot].activo = 1;

                printf("[EXITO] Programa '%s' cargado.\n", nombre_prog);
                printf("   RB: %d | RL: %d (Palabras: %d + Pila: %d)\n", 
                       tabla_procesos[slot].RB, tabla_procesos[slot].RL, num_palabras, TAMANO_PILA);
                printf("   Listo para ejecutar cuando quieras.\n");
            }
        }

        // --- EJECUTAR ---
        else if (opcion == 2) {
            listar_procesos();
            printf("ID a ejecutar: ");
            int id_sel = 0;
            scanf("%d", &id_sel);
            limpiar_buffer();

            int idx = -1;
            for(int i=0; i<MAX_PROCESOS; i++) {
                if (tabla_procesos[i].activo && tabla_procesos[i].id == id_sel) {
                    idx = i; break;
                }
            }

            if (idx != -1) {
                printf(">> Modo (0=Normal, 1=Debug): ");
                int modo = 0;
                scanf("%d", &modo);
                limpiar_buffer();

                // CAMBIO DE CONTEXTO
                cpu.RB = tabla_procesos[idx].RB;
                cpu.RL = tabla_procesos[idx].RL;
                cpu.psw.pc = cpu.RB + tabla_procesos[idx].pc_inicio;
                
                // SP inicia en el tope superior (RL) y crece hacia abajo
                // Nota: Restamos 1 porque RL es exclusivo o límite
                cpu.SP = (cpu.RL - cpu.RB) - 1; 

                cpu.interrupcion_pendiente = 0;
                cpu.ejecutando = 1;

                // A CORRER
                ejecutar_cpu(modo);

            } else {
                printf("[ERROR] ID no encontrado.\n");
            }
        }
    }

    // SALIDA
    cpu.ejecutando = 0;
    pthread_join(thread_timer_id, NULL);
    pthread_join(thread_dma_id, NULL);
    pthread_mutex_destroy(&cpu.mutex);
    logger_close();
    printf("Sistema apagado.\n");
    return 0;
}