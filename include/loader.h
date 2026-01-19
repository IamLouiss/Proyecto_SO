#ifndef LOADER_H
#define LOADER_H

// Carga un archivo .asm en la memoria simulada.
// filename: Ruta del archivo.
// direccion_carga: Dirección física inicial (RB).
// pc_inicial_out: Puntero donde se guardará el offset de inicio (_start).
// nombre_prog_out: Buffer para guardar el string de .NombreProg
// num_palabras_out: Puntero para guardar el valor de .NumeroPalabras
// Retorna: 0 si todo salió bien, -1 si hubo error.
int cargar_programa(const char *filename, int dir_carga, int *pc_inicial_out, char *nombre_prog_out, int *num_palabras_out);

#endif