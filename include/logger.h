#ifndef LOGGER_H
#define LOGGER_H

// Inicializa el sistema de logs (abre el archivo)
void logger_init(const char *filename);

// Escribe un mensaje (funciona IGUAL que printf)
void logger_log(const char *format, ...);

// Cierra el archivo de log
void logger_close();

#endif