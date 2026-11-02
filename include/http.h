#ifndef http_h
#define http_h
#include <stddef.h>
int analizar_solicitud_http(const char *buffer, size_t longitud, char *ruta, size_t capacidad_ruta);

#endif