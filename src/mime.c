#include <stdio.h>
#include <string.h>
#include "mime.h"

const char *obtener_tipo_mime(const char *ruta){
    const char *punto_extension = strrchr(ruta, '.');
    if(punto_extension == NULL){return "application/octet-stream";}
    const char *ultimo_slash = strrchr(ruta, '/');
    if(ultimo_slash != NULL && punto_extension < ultimo_slash){
        return "application/octet-stream";
    }
    if(strcmp(punto_extension, ".html") == 0){return "text/html";}
    if(strcmp(punto_extension, ".css") == 0){return "text/css";}
    if(strcmp(punto_extension, ".js") == 0){return "application/javascript";}
    if(strcmp(punto_extension, ".png") == 0){return "image/png";}
    if(strcmp(punto_extension, ".jpg") == 0){return "image/jpeg";}

    return "application/octet-stream";
}