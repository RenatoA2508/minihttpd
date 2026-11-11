#ifndef server_h
#define server_h
#include <stddef.h>
int crear_socket_serv(const char *puerto);
int enviar(int s, const char *buffer, size_t capacidad_buffer);

#endif