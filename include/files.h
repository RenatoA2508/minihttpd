#ifndef files_h
#define files_h

#include <stddef.h>

int solicitar_archivo(int *fd, const char *ruta, size_t *tamaño_archivo, char *ruta_efectiva, size_t tam_ruta_efectiva);

#endif