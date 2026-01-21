#ifndef LOADER_H
#define LOADER_H

// -- Funciones de carga:

// Abre el archivo del programa, lee las instrucciones y las guarda en el array de memoria
// Devuelve usando referencias el nombre, numero de palabras (instrucciones) y donde inicia el programa (_start)
// Retorna: 0 si todo salio bien, -1 si fallo
int cargar_programa(const char *nombre_archivo, int dir_carga, int *pc_inicial_out, char *nombre_prog_out, int *num_palabras_out);

// Funcion auxiliar que lee el _start, numero de palabras y nombre del programa
// De esta forma sabemos cuanto espacio va a ocupar el programa en memoria antes de cargarlo
int obtener_datos_programa(const char *nombre_archivo, char *nombre_out, int *tamano_out);

#endif