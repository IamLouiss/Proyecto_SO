#ifndef LOADER_H
#define LOADER_H

// Carga un archivo .asm en la memoria de la CPU
// Retorna 1 si tuvo éxito, 0 si falló
int cargar_programa(const char *nombre_archivo);

#endif