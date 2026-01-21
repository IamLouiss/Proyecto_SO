#ifndef LOGGER_H
#define LOGGER_H

// -- Sistema de registro
// Este modulo se utiliza para guardar todo lo que se realice en el sistema en un archivo .log
// que se guardara en la carpeta "logs" creada automaticamente al compilar

// Inicializa el sistema de logs (abre el archivo)
void logger_init(const char *nombre_archivo);

// Escribe un mensaje en el archivo .log
void logger_log(const char *format, ...);

// Cierra el archivo de log
void logger_close();

#endif