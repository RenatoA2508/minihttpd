#ifndef http_h
#define http_h
#include <stddef.h>

#define LIMITE_URI 2048
#define LIMITE_ENCABEZADOS 8192
#define LIMITE_PRIMERA_LINEA 4096
#define LIMITE_LINEA_ENCABEZADO 4096

enum resultados {
    HTTP_OK,
    HTTP_INCOMPLETA,
    HTTP_INVALIDA,
    HTTP_METODO_NO_PERMITIDO,
};

int analizar_solicitud_http(const char *buffer, size_t longitud, char *ruta, size_t capacidad_ruta, int *cerrar_conexion);

int generar_respuesta_http(int cod_estado, const char *mime, size_t longitud_cuerpo, int persistencia, char *buf_salida, size_t capacidad_buf_salida);


#endif